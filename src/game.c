/* Gameplay: player, enemy AI, weapons, grenades, C4, pickups, missions and rounds, camera */
#include "game.h"

Player P; Enemy EN[MAX_ENEMY]; Pickup PK[MAX_PICKUP]; Grenade GR[MAX_GREN]; C4 C4S[MAX_C4];
Part PT[MAX_PART]; Tracer TR[MAX_TRACER]; Mission M; GameState G; Input IN; Camera3D CAM;

const char *MISSION_NAMES[MAX_MISSIONS]={
 "FIRST LIGHT","BURIGANGA DAWN","SHAPLA STRIKE","MONSOON FURY","RICKSHAW RUN","HILSA RISING","PADMA THUNDER","KARNAPHULI",
 "SUNDARBAN SHADOW","NORTHERN WIND","JAMUNA FIRE","BOISHAKHI BLAZE","IRON LOTUS","GOLDEN DELTA","TIGER'S ROAR","RED CRESCENT",
 "SILENT MONSOON","CYCLONE EDGE","MIDNIGHT EXPRESS","SAFFRON STORM","SHAHEEN WINGS","DHAKA BLACKOUT","STEEL MONSOON","ECHO OF BANGLA",
 "CROWN OF FIRE","LAST BRIDGE","GREEN CYCLONE","BORDER FLAME","PHOENIX DAWN","OPERATION 2099"};

enum { PF_FIRE,PF_SMOKE,PF_DEBRIS,PF_SPARK,PF_FLASH };
static void ThrowGrenade(Vector3 pos,Vector3 vel,bool enemy,bool impact);
static float navTimer=0, c4Msg=0;

int MissionEnemyTotal(int idx){ return 10+idx/2; }
int MissionRounds(int idx){ return 2+idx/10; }
static float DiffDmg(void){ static const float d[3]={0.65f,1.0f,1.35f}; return d[SV.st.difficulty]; }
static float DiffHp(void){ static const float d[3]={0.8f,1.0f,1.25f}; return d[SV.st.difficulty]; }
static float DiffAcc(void){ static const float d[3]={0.7f,1.0f,1.2f}; return d[SV.st.difficulty]; }

/* ---------- messages ---------- */
void GameBanner(const char *s,Color c,float t){ snprintf(G.banner,sizeof(G.banner),"%s",s); G.bannerC=c; G.bannerT=t; }
void GameFeed(const char *s,Color c){
    for(int i=4;i>0;i--){ strcpy(G.feed[i],G.feed[i-1]); G.feedT[i]=G.feedT[i-1]; G.feedC[i]=G.feedC[i-1]; }
    snprintf(G.feed[0],sizeof(G.feed[0]),"%s",s); G.feedT[0]=4.0f; G.feedC[0]=c;
}
int GameAlive(void){ int n=0; for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active&&!EN[i].dead&&!EN[i].cop) n++; return n; }

/* ---------- effects ---------- */
static void SpawnPart(int kind,Vector3 p,Vector3 v,float life,float size,Color c){
    for(int i=0;i<MAX_PART;i++) if(!PT[i].active){ PT[i]=(Part){p,v,life,life,size,c,(unsigned char)kind,true}; return; }
}
static void AddTracer(Vector3 a,Vector3 b,Color c){
    for(int i=0;i<MAX_TRACER;i++) if(TR[i].life<=0){ TR[i]=(Tracer){a,b,0.09f,c}; return; }
}
static void Impact(Vector3 p,int n){
    for(int i=0;i<n;i++) SpawnPart(PF_SPARK,p,V3(RndR(-3,3),RndR(1,4),RndR(-3,3)),RndR(0.2f,0.5f),0.05f,(Color){255,210,120,255});
    SpawnPart(PF_SMOKE,p,V3(0,0.6f,0),0.5f,0.2f,(Color){150,150,150,140});
}
static void Muzzle(Vector3 p){ SpawnPart(PF_FLASH,p,V3(0,0,0),0.05f,0.22f,(Color){255,230,150,255}); }

/* ---------- damage ---------- */
#define FOOT_SPEED_MUL 3.0f   /* on-foot speed: 200% faster than the original */
static void HurtPlayer(float dmg,Vector3 from){
    if(!P.alive||M.stage==3) return;
    if(P.inVeh) dmg*=0.5f;
    if(P.adren>0) dmg*=0.7f;
    if(P.armor>0){ float ab=fminf(P.armor,dmg*0.6f); P.armor-=ab; dmg-=ab; }
    P.hp-=dmg; P.dmgTaken+=dmg; G.hurtT=0.6f; G.shake=fmaxf(G.shake,0.25f);
    G.hurtAng=AngleDiff(G.camYaw,atan2f(from.x-P.pos.x,from.z-P.pos.z));
    Sfx(SFX_HURT,0.8f,1.0f);
    if(P.hp<=0){
        if(M.free){
            if(P.inVeh){ VEH[P.vehIdx].player=false; VEH[P.vehIdx].parked=true; P.inVeh=false; }
            P.hp=P.maxHp; P.pos=M.start; P.pos.y=0; P.vy=0; P.score-=100; gWanted=0; gHeat=0; GameBanner("WASTED - RESPAWNING",(Color){255,60,60,255},3.0f);
        } else { P.hp=0; P.alive=false; GameEndMission(false); }
    }
}
static void SpawnPickup(Vector3 p,int type,int wid,float delay){
    for(int i=0;i<MAX_PICKUP;i++) if(!PK[i].active){ PK[i]=(Pickup){V3(p.x,0,p.z),type,true,Rnd01()*6,wid,delay}; return; }
}
static void DropPickup(Vector3 p,int wid){
    float r=Rnd01(); float hurt=1.0f-P.hp/P.maxHp;                /* 0 healthy .. 1 nearly dead */
    if(Rnd01()<0.18f+0.55f*hurt){ SpawnPickup(p,(hurt>0.55f&&Rnd01()<0.25f)?8:0,0,0); return; }   /* hurt? medkits drop much more often */
    if(r<0.16f) SpawnPickup(p,1,0,0); else if(r<0.24f) SpawnPickup(p,2,0,0); else if(r<0.36f) SpawnPickup(p,0,0,0);
    else if(r<0.44f) SpawnPickup(p,5,0,0); else if(r<0.58f) SpawnPickup(p,4,wid,0); else if(r<0.64f) SpawnPickup(p,7,0,0);
}
static void DamageEnemy(int i,float dmg,bool head,Vector3 knock,bool byPlayer,bool expl){
    Enemy *e=&EN[i]; if(e->dead||!e->active) return;
    bool unaware=(e->state==0&&e->spot<0.99f); if(e->state==0) e->alertT=2.0f;
    e->hp-=dmg; e->flash=1; e->state=1; e->alert=8; e->lostT=0; e->lastSeen=P.pos;
    for(int k=0;k<MAX_ENEMY;k++){ Enemy *o=&EN[k]; if(o->active&&!o->dead&&k!=i&&V3DistXZ(o->pos,e->pos)<35){ o->alert=6; o->state=1; o->lastSeen=P.pos; } }
    if(byPlayer){ G.hitT=0.25f; G.hitHead=head&&e->hp<=0; Sfx(head?SFX_HEAD:SFX_HIT,0.8f,1.0f); }
    Impact(V3(e->pos.x,e->pos.y+1.3f,e->pos.z),3);
    if(e->hp<=0){
        e->dead=true; e->deadT=0; e->vel=V3Add(V3Mul(V3Norm(knock),expl?9.0f:1.5f),V3(0,expl?5.0f:0,0)); if(!e->cop) M.killed++;
        if(e->cop&&byPlayer) CityCrime(1,"POLICE OFFICER KILLED");
        if(byPlayer){
            P.kills++; if(head) P.heads++;
            int pts=100+(head?50:0)+(expl?25:0); P.score+=pts;
            GameFeed(TextFormat("%s +%d",head?"HEADSHOT":(expl?"EXPLOSIVE KILL":"ENEMY DOWN"),pts),head?(Color){255,210,60,255}:(Color){240,240,240,255});
            if(unaware){ P.score+=100; GameFeed("STEALTH KILL +100",(Color){120,255,150,255}); }
        }
        DropPickup(e->pos,e->wid);
    }
}

static void Explode(Vector3 p,float radius,float dmg,bool byPlayer){
    CityExplosion(p,radius,dmg,byPlayer);
    Sfx3D(SFX_EXPLODE,p,0.85f+Rnd01()*0.3f);
    for(int i=0;i<26;i++){ float s=RndR(1.2f,3.6f); SpawnPart(PF_FIRE,p,V3(RndR(-1,1)*radius*0.45f,RndR(0.5f,1)*radius*0.45f,RndR(-1,1)*radius*0.45f),RndR(0.4f,0.9f),s,(Color){255,(unsigned char)RndR(110,200),40,255}); }
    for(int i=0;i<16;i++) SpawnPart(PF_SMOKE,p,V3(RndR(-2,2),RndR(1.5f,4),RndR(-2,2)),RndR(1.6f,3.0f),RndR(1.5f,3.5f),(Color){70,70,74,200});
    for(int i=0;i<18;i++) SpawnPart(PF_DEBRIS,p,V3(RndR(-1,1)*radius,RndR(0.5f,1.2f)*radius,RndR(-1,1)*radius),RndR(0.8f,1.6f),RndR(0.12f,0.3f),(Color){60,56,50,255});
    float dp=V3Dist(p,P.pos);
    G.shake=fmaxf(G.shake,Clampf(1.0f-dp/70.0f,0,1)*1.3f); if(dp<28) G.flashWhite=fmaxf(G.flashWhite,Clampf(1.0f-dp/28.0f,0,0.85f));
    G.flashT=0.3f; G.flashPos=p;
    Vector3 pe=V3Add(p,V3(0,0.6f,0));
    for(int i=0;i<MAX_ENEMY;i++){
        Enemy *e=&EN[i]; if(!e->active||e->dead) continue;
        Vector3 ch=V3(e->pos.x,e->pos.y+1.0f,e->pos.z); float d=V3Dist(p,ch); if(d>=radius) continue;
        float f=1.0f-d/radius, dm=dmg*f; Vector3 dir=V3Norm(V3Sub(ch,pe));
        if(WorldRayT(pe,dir,d,NULL)<d-0.6f) dm*=0.3f;
        if(dm>1) DamageEnemy(i,dm,false,V3Sub(ch,p),byPlayer,true);
    }
    if(P.alive){
        Vector3 ch=V3(P.pos.x,P.pos.y+1.0f,P.pos.z); float d=V3Dist(p,ch);
        if(d<radius){ float f=1.0f-d/radius,dm=dmg*f*(byPlayer?0.55f:0.8f); Vector3 dir=V3Norm(V3Sub(ch,pe));
            if(WorldRayT(pe,dir,d,NULL)<d-0.6f) dm*=0.3f; if(dm>1) HurtPlayer(dm,p); }
    }
    for(int i=0;i<nBarrel;i++){ Barrel *b=&BARREL[i]; if(b->alive&&b->fuse<0&&V3Dist(p,b->pos)<radius) b->fuse=0.12f+0.05f*(i%5); }
    if(M.cacheAlive){
        float d=V3Dist(p,BASE.cache);
        if(d<radius+5){ if(M.stage==2) M.cacheHP-=dmg*Clampf(1.0f-d/(radius+5),0,1)*1.25f;
            else if(c4Msg<=0){ GameBanner("ELIMINATE ALL HOSTILES FIRST",(Color){255,120,80,255},2.5f); c4Msg=3; } }
    }
}
static void ExplodeBarrelUpdate(float dt){
    for(int i=0;i<nBarrel;i++){ Barrel *b=&BARREL[i]; if(!b->alive||b->fuse<0) continue;
        b->fuse-=dt; if(b->fuse<=0){ b->alive=false; OBS[b->obs].active=false; Explode(V3(b->pos.x,0.6f,b->pos.z),9,150,true); } }
}

/* ---------- hit scan ---------- */
static bool HitScan(Vector3 o,Vector3 d,float range,float *tOut,int *enemy,bool *head,int *barrel,int *ck,int *ci){
    int oi; float tw=WorldRayT(o,d,range,&oi); *enemy=-1; *head=false; *barrel=-1;
    float best=tw;
    for(int i=0;i<MAX_ENEMY;i++){ const Enemy *e=&EN[i]; if(!e->active||e->dead) continue;
        float hw=(e->type==EN_HEAVY)?0.42f:0.33f, t;
        if(RayAABB(o,d,V3(e->pos.x-hw,e->pos.y,e->pos.z-hw),V3(e->pos.x+hw,e->pos.y+1.85f,e->pos.z+hw),&t)&&t<best){
            best=t; *enemy=i; *head=(o.y+d.y*t)>e->pos.y+1.5f; }
    }
    *ck=0; *ci=-1; { float ct; int k2,i2; if(CityHitScan(o,d,best,&ct,&k2,&i2)&&ct<best){ best=ct; *enemy=-1; *head=false; *ck=k2; *ci=i2; } }
    if(*enemy<0&&*ck==0&&oi>=0&&OBS[oi].kind==OB_BARREL) *barrel=OBS[oi].style;
    *tOut=best; return *enemy>=0;
}
static Vector3 Perturb(Vector3 f,float deg){
    if(deg<=0.001f) return f;
    Vector3 r=V3Norm(V3(f.z,0,-f.x));
    Vector3 u=V3(f.y*r.z-f.z*r.y,f.z*r.x-f.x*r.z,f.x*r.y-f.y*r.x);
    float a=RndR(0,2*PI), m=sqrtf(Rnd01())*deg*DEG2RAD;
    return V3Norm(V3Add(f,V3Add(V3Mul(r,cosf(a)*tanf(m)),V3Mul(u,sinf(a)*tanf(m)))));
}
static Vector3 CamForward(void){ return V3(sinf(G.camYaw)*cosf(G.camPitch),sinf(G.camPitch),cosf(G.camYaw)*cosf(G.camPitch)); }

/* ---------- player actions ---------- */
static void StartReload(void){
    const WeaponDef *w=&WEAPONS[P.wid[P.cw]];
    if(P.reloading||P.mag[P.cw]>=w->mag||P.res[P.cw]<=0) return;
    P.reloading=true; P.reloadT=w->reload; Sfx(SFX_RELOAD,0.8f,1.0f);
}
static void PlayerFire(void){
    const WeaponDef *w=&WEAPONS[P.wid[P.cw]]; const CharDef *ch=&CHARS[SV.selChar];
    P.mag[P.cw]--; P.fireT=w->interval; P.muzzleT=0.06f;
    Vector3 f=CamForward(), head=V3(P.pos.x,P.pos.y+1.5f,P.pos.z);
    Vector3 o=V3Add(CAM.position,V3Mul(f,fmaxf(1.0f,V3Dot(V3Sub(head,CAM.position),f))));
    if(SV.st.aimAssist&&UiTouchMode()){
        float bestA=0.09f; int bi=-1;
        for(int i=0;i<MAX_ENEMY;i++){ const Enemy *e=&EN[i]; if(!e->active||e->dead) continue;
            Vector3 d=V3Norm(V3Sub(V3(e->pos.x,e->pos.y+1.2f,e->pos.z),o)); float a=acosf(Clampf(V3Dot(d,f),-1,1));
            if(a<bestA&&V3Dist(o,e->pos)<w->range){ bestA=a; bi=i; } }
        if(bi>=0){ Vector3 d=V3Norm(V3Sub(V3(EN[bi].pos.x,EN[bi].pos.y+1.2f,EN[bi].pos.z),o)); f=V3Norm(V3Add(V3Mul(f,0.35f),V3Mul(d,0.65f))); }
    }
    Vector3 mz=MuzzlePos(P.pos,P.yaw,P.pitch,P.wid[P.cw],true);
    if(w->kind==1){
        float tw=WorldRayT(o,f,150.0f,NULL); Vector3 aim=V3Add(o,V3Mul(f,tw)); Vector3 dd=V3Norm(V3Sub(aim,mz));
        ThrowGrenade(mz,V3Mul(dd,40.0f),false,true); Muzzle(mz); Sfx(w->sfx,1.0f,1.0f); CityAlarm(P.pos,70);
        G.camPitch+=0.03f; G.shake=fmaxf(G.shake,0.25f); P.recoil=fminf(P.recoil+w->recoil,8.0f);
        for(int i=0;i<MAX_ENEMY;i++){ Enemy *e=&EN[i]; if(e->active&&!e->dead&&e->state==0&&V3DistXZ(e->pos,P.pos)<90){ e->alert=5; e->state=1; e->lastSeen=P.pos; e->alertT=2; } }
        return;
    }
    float spread=w->spread*(P.aiming?0.45f:1.0f)*(P.speedN>0.3f?1.5f:1.0f)+P.recoil*0.25f;
    float dmgMul=ch->dmgMul*(1.0f+0.07f*(P.rank-1));
    for(int k=0;k<w->pellets;k++){
        Vector3 d=Perturb(f,spread); float t; int ei,bi,ck,ci; bool hd;
        bool hit=HitScan(o,d,w->range,&t,&ei,&hd,&bi,&ck,&ci);
        Vector3 end=V3Add(o,V3Mul(d,t));
        if(k<3) AddTracer(mz,end,(Color){255,235,160,255});
        if(hit){
            float dm=w->dmg*dmgMul; if(hd) dm*=2.2f*ch->headMul; if(w->pellets>1) dm*=Clampf(1.0f-t/w->range*0.8f,0.15f,1);
            DamageEnemy(ei,dm,hd,d,true,false);
        } else if(ck>0){ CityDamage(ck,ci,w->dmg*dmgMul,d,true); Impact(end,2);
        } else if(bi>=0){ BARREL[bi].hp-=w->dmg*dmgMul; Impact(end,4); if(BARREL[bi].hp<=0&&BARREL[bi].fuse<0) BARREL[bi].fuse=0.08f; }
        else Impact(end,3);
    }
    CityAlarm(P.pos,50);
    Muzzle(mz); Sfx(w->sfx,1.0f,1.0f+RndR(-0.04f,0.04f));
    G.camPitch+=w->recoil*0.006f*(P.aiming?0.6f:1.0f); P.recoil=fminf(P.recoil+w->recoil,8.0f);
    for(int i=0;i<MAX_ENEMY;i++){ Enemy *e=&EN[i]; if(e->active&&!e->dead&&e->state==0&&V3DistXZ(e->pos,P.pos)<70*ch->stealth+20){ e->alert=5; e->state=1; e->lastSeen=P.pos; } }
}
static void ThrowGrenade(Vector3 pos,Vector3 vel,bool enemy,bool impact){
    for(int i=0;i<MAX_GREN;i++) if(!GR[i].active){ GR[i]=(Grenade){pos,vel,impact?4.0f:2.4f,true,enemy,impact}; return; }
}
static bool GrenadeNear(Vector3 p){
    for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active&&!EN[i].dead&&V3Dist(p,V3(EN[i].pos.x,EN[i].pos.y+1.0f,EN[i].pos.z))<1.3f) return true;
    for(int i=0;i<MAX_CIT;i++) if(CIT[i].active&&!CIT[i].dead&&V3Dist(p,V3(CIT[i].pos.x,1.0f,CIT[i].pos.z))<1.1f) return true;
    for(int i=0;i<MAX_VEH;i++) if(VEH[i].active&&!(VEH[i].player&&P.inVeh)&&V3DistXZ(p,VEH[i].pos)<(VEH[i].type==3?5.0f:2.4f)&&p.y<3.0f) return true;
    return false;
}
static void UpdateGrenades(float dt){
    for(int i=0;i<MAX_GREN;i++){ Grenade *g=&GR[i]; if(!g->active) continue;
        g->fuse-=dt; g->vel.y-=(g->impact?5.0f:18.0f)*dt;
        Vector3 np=V3Add(g->pos,V3Mul(g->vel,dt));
        bool hitX=false,hitY=false,hitZ=false;
        for(int k=0;k<nObs;k++){ const Obs *o=&OBS[k]; if(!o->active||o->kind==OB_WATER) continue;
            const float r=0.12f;
            if(np.x>o->mn.x-r&&np.x<o->mx.x+r&&np.y>o->mn.y-r&&np.y<o->mx.y+r&&np.z>o->mn.z-r&&np.z<o->mx.z+r){
                bool inX=g->pos.x>o->mn.x-r&&g->pos.x<o->mx.x+r, inY=g->pos.y>o->mn.y-r&&g->pos.y<o->mx.y+r, inZ=g->pos.z>o->mn.z-r&&g->pos.z<o->mx.z+r;
                if(!inX) hitX=true; else if(!inZ) hitZ=true; else if(!inY) hitY=true; else hitY=true;
                if(hitX&&hitY&&hitZ) break; } }
        bool contact=hitX||hitY||hitZ||np.y<0.13f;
        if(hitX){ g->vel.x*=-0.4f; np.x=g->pos.x; } if(hitZ){ g->vel.z*=-0.4f; np.z=g->pos.z; }
        if(hitY){ g->vel.y*=-0.35f; g->vel.x*=0.6f; g->vel.z*=0.6f; np.y=g->pos.y; }
        if(np.y<0.12f){ np.y=0.12f; if(g->vel.y<0){ g->vel.y*=-0.35f; g->vel.x*=0.65f; g->vel.z*=0.65f; if(fabsf(g->vel.y)<0.8f) g->vel.y=0; } }
        g->pos=np;
        if(g->impact){ if(Rnd01()<0.5f) SpawnPart(PF_SMOKE,g->pos,V3(0,0.3f,0),0.6f,0.25f,(Color){200,200,200,150}); if(contact||GrenadeNear(g->pos)) g->fuse=0; }
        if(g->fuse<=0){ g->active=false; Explode(g->pos,10,g->enemy?110:170,!g->enemy); }
    }
}
static void UpdateC4(float dt){
    for(int i=0;i<MAX_C4;i++){ C4 *c=&C4S[i]; if(!c->active) continue; c->beep-=dt; if(c->beep<=0){ c->beep=1.0f; Sfx3D(SFX_BEEP,c->pos,0.8f); } }
}
static void GiveWeapon(int wid,bool *took){
    const WeaponDef *w=&WEAPONS[wid];
    for(int s=0;s<2;s++) if(P.wid[s]==wid){
        if(P.res[s]>=w->reserve*2&&P.mag[s]>=w->mag) return;
        P.res[s]=(P.res[s]+w->mag*2>w->reserve*2)?w->reserve*2:P.res[s]+w->mag*2; *took=true;
        GameFeed(TextFormat("+AMMO %s",w->name),(Color){255,220,120,255}); return; }
    int old=P.wid[P.cw]; SpawnPickup(P.pos,4,old,2.5f);
    P.wid[P.cw]=wid; P.mag[P.cw]=w->mag; P.res[P.cw]=w->reserve/2+w->mag; P.reloading=false; P.fireT=0.3f;
    *took=true; GameFeed(TextFormat("PICKED UP %s",w->name),(Color){255,210,90,255});
}
static void Pickups(float dt){
    for(int i=0;i<MAX_PICKUP;i++){ Pickup *k=&PK[i]; if(!k->active) continue; k->t+=dt;
        if(k->delay>0){ k->delay-=dt; continue; }
        if(V3DistXZ(k->pos,P.pos)>1.8f) continue;
        bool took=false;
        switch(k->type){
            case 0: if(P.hp<P.maxHp){ P.hp=fminf(P.maxHp,P.hp+55); took=true; GameFeed("+55 HEALTH",(Color){120,255,150,255}); } break;
            case 1: { bool any=false; for(int s=0;s<2;s++){ const WeaponDef *w=&WEAPONS[P.wid[s]]; if(P.res[s]<w->reserve*2){ P.res[s]+=w->mag*2; any=true; } } if(any){ took=true; GameFeed("+AMMO",(Color){255,220,120,255}); } } break;
            case 2: if(P.gren<9){ P.gren=(P.gren+2>9)?9:P.gren+2; took=true; GameFeed("+2 GRENADES",(Color){200,230,140,255}); } break;
            case 3: if(P.bombs<5){ P.bombs++; took=true; GameFeed("+1 C4 CHARGE",(Color){255,160,90,255}); } break;
            case 4: if(!P.inVeh) GiveWeapon(k->wid,&took); break;
            case 8: if(P.hp<P.maxHp){ P.hp=P.maxHp; took=true; GameFeed("FIRST AID KIT - FULL HEALTH",(Color){120,255,150,255}); } break;
            case 5: if(P.armor<100){ P.armor=fminf(100,P.armor+50); took=true; GameFeed("+50 BODY ARMOR",(Color){120,180,255,255}); } break;
            case 6: P.adren=12.0f; took=true; GameFeed("ADRENALINE: FASTER + TOUGHER",(Color){220,140,255,255}); break;
            case 7: P.score+=300; took=true; GameFeed("INTEL RECOVERED +300",(Color){255,210,60,255}); break;
        }
        if(took){ k->active=false; Sfx(SFX_PICK,0.8f,1.0f); }
    }
}

/* ---------- player update ---------- */
static void PlayerUpdate(float dt){
    const CharDef *ch=&CHARS[SV.selChar];
    G.camYaw-=IN.look_x; G.camPitch-=IN.look_y*(SV.st.invertY?-1.0f:1.0f);
    G.camPitch=Clampf(G.camPitch,-0.62f,0.85f);
    P.adren=fmaxf(0,P.adren-dt);
    if(IN.use) CityUse();
    if(P.inVeh){ P.aiming=false; CityDrive(dt); P.fireT-=dt; Pickups(dt); return; }
    Vector3 f=YawDir(G.camYaw), r=YawRight(G.camYaw);
    Vector3 wish=V3Add(V3Mul(f,IN.move_y),V3Mul(r,IN.move_x)); float wl=V3Len(wish); if(wl>1) wish=V3Mul(wish,1.0f/wl),wl=1;
    P.aiming=IN.aim;
    P.sprinting=IN.sprint&&IN.move_y>0.2f&&!P.aiming&&wl>0.2f;
    float spd=7.0f*FOOT_SPEED_MUL*ch->spdMul*(P.adren>0?1.3f:1.0f)*(P.sprinting?1.75f:1.0f)*(P.aiming?0.65f:1.0f)*(P.reloading?0.9f:1.0f);
    if(P.hp<P.maxHp*0.25f) spd*=0.8f;                 /* limping when badly hurt */
    Vector3 before=P.pos;
    float g=0; int steps=(int)ceilf(spd*dt/0.35f); if(steps<1) steps=1;
    for(int s=0;s<steps;s++){ P.pos.x+=wish.x*spd*dt/steps; P.pos.z+=wish.z*spd*dt/steps; MoveCollide(&P.pos,0.42f,&g); }
    P.vy-=22.0f*dt; P.pos.y+=P.vy*dt;
    if(P.pos.y<=g){
        if(!P.onGround&&P.vy<-15.0f){ float fall=(-P.vy-15.0f)*6.0f; HurtPlayer(fall,P.pos); GameFeed("HARD LANDING",(Color){255,120,90,255}); }
        P.pos.y=g; P.vy=0; P.onGround=true;
    } else P.onGround=false;
    if(IN.jump&&P.onGround){ P.vy=7.2f; P.onGround=false; }
    float moved=V3DistXZ(before,P.pos)/fmaxf(dt,0.0001f);
    P.speedN=Clampf(moved/8.0f,0,1.3f); P.walk+=P.speedN*dt*(P.sprinting?11.0f:8.0f);
    P.stepT-=dt; (void)P.stepT;
    bool firing=IN.fire;
    if(wl>0.15f||P.aiming||firing||fabsf(AngleDiff(P.yaw,G.camYaw))>1.2f) P.yaw+=AngleDiff(P.yaw,G.camYaw)*fminf(1.0f,dt*14.0f);
    P.pitch=Clampf(G.camPitch,-0.5f,0.7f);
    P.recoil=fmaxf(0,P.recoil-dt*6.0f); P.fireT-=dt; P.muzzleT-=dt;
    if(ch->regen>0&&P.hp<P.maxHp&&P.alive) P.hp=fminf(P.maxHp,P.hp+ch->regen*dt);
    if(IN.swap){ P.cw^=1; P.reloading=false; Sfx(SFX_CLICK,0.6f,1.2f); }
    const WeaponDef *w=&WEAPONS[P.wid[P.cw]];
    if(P.reloading){ P.reloadT-=dt; if(P.reloadT<=0){ int need=w->mag-P.mag[P.cw],take=need<P.res[P.cw]?need:P.res[P.cw]; P.mag[P.cw]+=take; P.res[P.cw]-=take; P.reloading=false; } }
    if(IN.reload) StartReload();
    bool want=IN.fire&&(w->automatic||IN.firePressed);
    if(!P.reloading&&P.fireT<=0&&want){ if(P.mag[P.cw]>0) PlayerFire(); else { if(IN.firePressed) Sfx(SFX_EMPTY,0.7f,1.0f); StartReload(); } }
    if(IN.grenade&&P.gren>0){ P.gren--; Sfx(SFX_PIN,0.9f,1.0f);
        Vector3 d=CamForward(); ThrowGrenade(V3Add(V3(P.pos.x,P.pos.y+1.6f,P.pos.z),V3Mul(YawDir(G.camYaw),0.6f)),V3Add(V3Mul(d,16.5f),V3(0,3.2f,0)),false,false); }
    if(IN.c4&&P.bombs>0&&P.onGround){
        for(int i=0;i<MAX_C4;i++) if(!C4S[i].active){ Vector3 p=V3Add(P.pos,V3Mul(YawDir(P.yaw),0.9f)); p.y=0; C4S[i]=(C4){p,true,0.2f}; P.bombs--; Sfx(SFX_BEEP,0.9f,1.0f); GameFeed("C4 PLANTED",(Color){255,160,90,255}); break; } }
    if(IN.det){ bool any=false; for(int i=0;i<MAX_C4;i++) if(C4S[i].active){ C4S[i].active=false; Explode(V3(C4S[i].pos.x,0.4f,C4S[i].pos.z),14,520,true); any=true; } if(!any) Sfx(SFX_EMPTY,0.6f,1.0f); }
    Pickups(dt);
}

void GameExplode(Vector3 p,float radius,float dmg,bool byPlayer){ Explode(p,radius,dmg,byPlayer); }
void GameHurtEnemy(int i,float dmg,bool head,Vector3 knock,bool byPlayer,bool expl){ DamageEnemy(i,dmg,head,knock,byPlayer,expl); }
void GameSmoke(Vector3 p){ SpawnPart(PF_SMOKE,p,V3(RndR(-0.4f,0.4f),RndR(1.2f,2.4f),RndR(-0.4f,0.4f)),RndR(1.0f,1.8f),RndR(0.6f,1.2f),(Color){60,60,64,170}); }

/* ---------- enemies ---------- */
static Look MercLook(int type){
    static const Color skins[5]={{150,104,72,255},{171,120,85,255},{110,72,50,255},{196,148,110,255},{125,84,58,255}};
    static const Color shirts[5]={{40,44,40,255},{70,72,60,255},{60,60,66,255},{86,70,52,255},{34,38,50,255}};
    Look l; int v=GetRandomValue(0,4);
    l.skin=skins[GetRandomValue(0,4)]; l.shirt=shirts[v]; l.pants=ColMul(shirts[(v+2)%5],0.9f); l.vest=ColMul(shirts[(v+1)%5],0.7f);
    l.hat=ColMul(shirts[v],0.8f); l.hair=(Color){20,16,14,255}; l.hairType=1; l.beard=GetRandomValue(0,2)==0; l.shades=GetRandomValue(0,3)==0;
    int hts[4]={5,2,1,3}; l.hatType=(unsigned char)hts[GetRandomValue(0,3)];
    if(type==EN_HEAVY){ l.hatType=2; l.vest=(Color){60,62,58,255}; }
    if(type==EN_COMMANDER){ l.hatType=6; l.hat=(Color){120,25,30,255}; l.vest=(Color){150,120,50,255}; l.shirt=(Color){28,28,32,255}; l.pants=(Color){28,28,32,255}; l.beard=true; l.shades=true; }
    if(type==EN_SNIPER){ l.hatType=3; l.hat=(Color){60,70,50,255}; l.shirt=(Color){60,72,52,255}; l.pants=(Color){56,66,48,255}; }
    return l;
}
void GameHurtPlayer(float dmg,Vector3 from){ HurtPlayer(dmg,from); }
static int AllocEnemy(void){
    for(int i=0;i<MAX_ENEMY;i++) if(!EN[i].active) return i;
    int b=-1; float bt=-1; for(int i=0;i<MAX_ENEMY;i++) if(EN[i].dead&&EN[i].deadT>bt){ bt=EN[i].deadT; b=i; }
    return b;
}
static void SpawnEnemy(int type,Vector3 pos,bool alerted,bool stat){
    int i=AllocEnemy(); if(i<0) return;
    Enemy *e=&EN[i]; memset(e,0,sizeof(*e));
    static const int wid[6]={W_AK47,W_MP5,W_SHOTGUN,W_SVD,W_PKM,W_M4A1};
    static const float hp[6]={70,55,90,55,220,380}, spd[6]={4.6f,5.6f,5.0f,3.2f,3.0f,4.4f}, pmin[6]={14,6,4,40,20,12}, pmax[6]={32,16,10,70,34,28};
    float scale=(1.0f+0.03f*M.idx)*DiffHp();
    e->active=true; e->type=type; e->wid=wid[type]; e->maxHp=e->hp=hp[type]*scale; e->speed=spd[type]; e->prefMin=pmin[type]; e->prefMax=pmax[type];
    e->pos=pos; e->home=pos; e->dest=pos; e->isStatic=stat; e->yaw=RndR(0,2*PI); e->look=MercLook(type); e->strafeDir=(GetRandomValue(0,1)?1.0f:-1.0f);
    e->fireT=RndR(0.5f,1.2f); e->grenT=RndR(6,12);
    if(alerted){ e->state=1; e->alert=8; e->lastSeen=P.pos; }
}
static Look PoliceLook(void){
    Look l; memset(&l,0,sizeof(l));
    l.skin=(Color){160,112,78,255}; l.shirt=(Color){24,38,86,255}; l.pants=(Color){14,16,28,255}; l.vest=(Color){10,10,14,255};
    l.hat=(Color){12,16,36,255}; l.hair=(Color){20,16,14,255}; l.hatType=1; l.hairType=1; l.shades=true; return l;
}
void GameSpawnCop(Vector3 pos){
    int n=0; for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active&&!EN[i].dead&&EN[i].cop) n++;
    if(n>=10) return;
    int i=AllocEnemy(); if(i<0) return;
    SpawnEnemy((n%3==2)?EN_RIFLE:EN_SMG,pos,true,false);
    Enemy *e=&EN[i]; e->cop=true; e->look=PoliceLook(); e->maxHp=e->hp=55.0f+8.0f*gWanted; e->lastSeen=P.pos;
}
static void EnemyShoot(Enemy *e,float dist){
    const WeaponDef *w=&WEAPONS[e->wid];
    Vector3 pc=V3(P.pos.x,P.pos.y+1.15f,P.pos.z);
    Vector3 mz=MuzzlePos(e->pos,e->yaw,0,e->wid,true); mz.y=e->pos.y+1.35f;
    Vector3 d=V3Norm(V3Sub(pc,mz)); float dl=V3Dist(pc,mz);
    static const float accB[6]={0.40f,0.35f,0.45f,0.52f,0.30f,0.45f}, rng[6]={95,65,38,140,85,95};
    float acc=accB[e->type]*DiffAcc()*Clampf(1.2f-dist/rng[e->type],0.12f,1.0f)*(P.speedN>0.5f?0.8f:1.0f)*(P.aiming?0.9f:1.0f)*(1.0f+0.01f*M.idx);
    float t=WorldRayT(mz,d,dl+1.0f,NULL); bool clear=t>=dl-0.4f;
    float dmgBase=w->dmg*0.21f*DiffDmg()*(1.0f+0.02f*M.idx); if(e->type==EN_SNIPER) dmgBase*=1.1f;
    int pel=(w->pellets>1)?5:1;
    for(int k=0;k<pel;k++){
        bool hit=clear&&Rnd01()<acc*(pel>1?0.6f:1.0f);
        Vector3 end; if(hit){ end=pc; HurtPlayer(dmgBase,e->pos); }
        else{ Vector3 o=V3(RndR(-1,1),RndR(-0.8f,0.8f),RndR(-1,1)); o=V3Mul(V3Norm(o),RndR(0.5f,1.8f));
            Vector3 dd=V3Norm(V3Add(V3Sub(pc,mz),o)); float tt=WorldRayT(mz,dd,clear?dl+25.0f:t,NULL); end=V3Add(mz,V3Mul(dd,tt)); if(!clear||tt<dl+25.0f) Impact(end,2); }
        if(k<2) AddTracer(mz,end,(Color){255,150,70,255});
    }
    CityAlarm(e->pos,35);
    Muzzle(mz); Sfx3D(w->sfx,e->pos,1.0f+RndR(-0.08f,0.08f));
}
static void EnemyUpdate(Enemy *e,int idx,float dt){
    (void)idx;
    if(e->dead){
        e->deadT+=dt; e->fall=fminf(1.0f,e->fall+dt*3.2f);
        e->pos=V3Add(e->pos,V3Mul(e->vel,dt)); e->vel=V3Mul(e->vel,fmaxf(0,1.0f-dt*3.0f));
        if(!e->isStatic){ e->pos.y=fmaxf(0,e->pos.y+e->vel.y*dt); e->vel.y-=15*dt; if(e->pos.y<0) e->pos.y=0; MoveCollide(&e->pos,0.3f,NULL); }
        e->flash=fmaxf(0,e->flash-dt*3); if(e->deadT>12) e->active=false; return;
    }
    Vector3 eye=V3(e->pos.x,e->pos.y+1.55f,e->pos.z), pc=V3(P.pos.x,P.pos.y+1.2f,P.pos.z);
    float dist=V3DistXZ(e->pos,P.pos); const CharDef *ch=&CHARS[SV.selChar];
    e->fireT-=dt; e->grenT-=dt; e->strafeT-=dt; e->sideT-=dt; e->flash=fmaxf(0,e->flash-dt*4); e->alert-=dt;
    e->los-=dt; if(e->alertT>0) e->alertT-=dt;
    if(e->los<=0){
        e->los=0.12f+Rnd01()*0.1f; e->canSee=false;
        if(P.alive){
            float sight=((e->type==EN_SNIPER)?130.0f:78.0f)*ch->stealth*(e->state==1?1.6f:1.0f);
            if(dist<sight){
                Vector3 d=V3Sub(pc,eye); float dl=V3Len(d); d=V3Norm(d);
                Vector3 hd=V3Norm(V3(d.x,0,d.z));
                bool fov=V3Dot(YawDir(e->yaw),hd)>0.3f||dist<10||e->state==1;
                if(fov) e->canSee=WorldRayT(eye,d,dl,NULL)>=dl-0.5f;
            }
        }
    }
    bool moving=false; Vector3 mv=V3(0,0,0); float sp=0;
    if(e->state==0){
        if(e->canSee){
            float noise=P.inVeh?1.6f:(P.sprinting?1.5f:(P.speedN<0.1f?0.6f:1.0f));
            e->spot+=dt*noise*(0.35f+14.0f/(dist+8.0f))/ch->stealth;
            if(e->spot>=1.0f){ e->spot=1.0f; e->state=1; e->alert=8; e->fireT=RndR(0.4f,0.9f); e->lastSeen=P.pos; e->alertT=2.0f; Sfx3D(SFX_ALERT,e->pos,1.0f); }
        } else e->spot=fmaxf(0,e->spot-dt*0.3f);
        if(e->alert>0){ e->state=1; e->lastSeen=P.pos; e->alertT=2.0f; }
        if(e->spot>0.3f&&e->state==0){ e->yaw+=AngleDiff(e->yaw,atan2f(P.pos.x-e->pos.x,P.pos.z-e->pos.z))*fminf(1,dt*3.5f); }
        else if(!e->isStatic){
            if(V3DistXZ(e->pos,e->dest)<1.5f||e->stuck>1.5f){
                for(int k=0;k<8;k++){ Vector3 c=V3(e->home.x+RndR(-16,16),0,e->home.z+RndR(-16,16)); if(WorldFree(c.x,c.z,0.8f)){ e->dest=c; break; } }
                e->stuck=0; }
            mv=V3Norm(V3Sub(e->dest,e->pos)); mv.y=0; sp=e->speed*0.35f; moving=true;
            if(V3Len(mv)>0.1f) e->yaw+=AngleDiff(e->yaw,atan2f(mv.x,mv.z))*fminf(1,dt*5);
        } else if(e->canSee) e->yaw+=AngleDiff(e->yaw,atan2f(P.pos.x-e->pos.x,P.pos.z-e->pos.z))*fminf(1,dt*4);
    } else {
        if(e->canSee){ e->lastSeen=P.pos; e->lostT=0; e->alert=6; } else e->lostT+=dt;
        Vector3 tgt=e->canSee?P.pos:e->lastSeen;
        e->yaw+=AngleDiff(e->yaw,atan2f(tgt.x-e->pos.x,tgt.z-e->pos.z))*fminf(1,dt*9);
        if(e->lostT>12&&e->alert<=0){ e->state=0; e->spot=0; }
        if(!e->isStatic){
            Vector3 toP=V3Norm(V3(P.pos.x-e->pos.x,0,P.pos.z-e->pos.z)); moving=true;
            if(e->canSee){
                if(dist>e->prefMax){ mv=toP; sp=e->speed; }
                else if(dist<e->prefMin){ mv=V3Mul(toP,-1); sp=e->speed*0.7f; }
                else{ if(e->strafeT<=0){ e->strafeT=RndR(1.2f,3.0f); e->strafeDir=-e->strafeDir; } mv=V3Mul(V3(toP.z,0,-toP.x),e->strafeDir); sp=e->speed*0.55f; }
            } else {
                Vector3 fd; if(V3DistXZ(e->pos,e->lastSeen)>3.0f){ if(NavDir(e->pos,&fd)) mv=fd; else mv=V3Norm(V3(e->lastSeen.x-e->pos.x,0,e->lastSeen.z-e->pos.z)); sp=e->speed; }
                else{ moving=false; }
            }
        }
        if(P.alive&&M.stage!=3&&e->canSee&&e->fireT<=0&&dist<((e->type==EN_SNIPER)?140:(e->type==EN_SHOTGUN)?38:95)){
            if(e->burst<=0) e->burst=(e->type==EN_SMG)?5:(e->type==EN_HEAVY)?8:(e->type==EN_RIFLE||e->type==EN_COMMANDER)?3:1;
            EnemyShoot(e,dist); e->burst--; const WeaponDef *w=&WEAPONS[e->wid];
            e->fireT=(e->burst>0)?w->interval*1.15f:w->interval+((e->type==EN_SNIPER)?RndR(1.8f,2.8f):RndR(0.9f,1.8f));
        }
        if(M.idx>=6&&e->type!=EN_SNIPER&&!e->isStatic&&e->canSee&&e->grenT<=0&&dist>14&&dist<42&&P.alive&&M.stage!=3){
            float T=Clampf(dist/15.0f,0.8f,2.0f); Vector3 dd=V3Norm(V3(P.pos.x-e->pos.x,0,P.pos.z-e->pos.z));
            Vector3 v=V3Mul(dd,dist/T); v.y=(-1.6f+9.0f*T*T)/T; v.x+=RndR(-1.5f,1.5f); v.z+=RndR(-1.5f,1.5f);
            ThrowGrenade(V3(e->pos.x,e->pos.y+1.6f,e->pos.z),v,true,false); e->grenT=RndR(9,16); G.warnGren=3.0f; Sfx3D(SFX_PIN,e->pos,1.0f);
        }
    }
    if(moving&&sp>0&&!e->isStatic){
        if(e->sideT>0){ mv=V3(mv.z*e->strafeDir,0,-mv.x*e->strafeDir); }
        Vector3 old=e->pos; e->pos=V3Add(e->pos,V3Mul(mv,sp*dt)); e->pos.y=0; MoveCollide(&e->pos,0.4f,NULL);
        float mvd=V3DistXZ(old,e->pos)/fmaxf(dt,0.0001f);
        if(mvd<sp*0.3f){ e->stuck+=dt; if(e->stuck>0.6f){ e->sideT=0.7f; e->strafeDir=(GetRandomValue(0,1)?1.0f:-1.0f); e->stuck=0.0f; } } else if(e->sideT<=0) e->stuck=fmaxf(0,e->stuck-dt);
        e->speedN=Clampf(mvd/4.5f,0,1.2f); e->walk+=e->speedN*dt*8.0f;
    } else e->speedN=Lerpf(e->speedN,0,fminf(1,dt*8));
}

/* ---------- missions ---------- */
static bool SpotIn(float x0,float z0,float x1,float z1,Vector3 *out){
    for(int k=0;k<40;k++){ float x=RndR(x0,x1),z=RndR(z0,z1); if(WorldFree(x,z,1.0f)&&!IsWaterAt(x,z)){ *out=V3(x,0,z); return true; } }
    return false;
}
static int PickType(void){
    float r=Rnd01()*100; int m=M.idx;
    if(m>=9&&r<9) return EN_HEAVY; if(m>=6&&r<17) return EN_SNIPER; if(m>=3&&r<32) return EN_SHOTGUN; if(r<58) return EN_SMG; return EN_RIFLE;
}
static int RoundCount(int r){
    int n1=(M.total+1)/2; if(r==0) return n1;
    int rest=M.total-n1, per=rest/(M.rounds-1);
    return (r==M.rounds-1)?rest-per*(M.rounds-2):per;
}
static void SpawnRound(int r){
    int n=RoundCount(r); bool last=(r==M.rounds-1);
    if(r==0){
        int placed=0;
        if(M.idx>=2) for(int t=0;t<4&&placed<n;t++){
            float tx=BASE.x0+((t&1)?153:17), tz=BASE.z0+((t&2)?153:17);
            SpawnEnemy((M.idx>=3)?EN_SNIPER:EN_RIFLE,V3(tx,9.0f,tz),false,true); placed++; }
        int guards=(int)((n-placed)*0.65f);
        for(int k=0;k<guards;k++){ Vector3 p; if(SpotIn(BASE.x0+20,BASE.z0+20,BASE.x0+150,BASE.z0+150,&p)) SpawnEnemy(PickType(),p,false,false); placed++; }
        while(placed<n){
            Vector3 p; float cx=BASE.center.x,cz=BASE.center.z; bool ok=false;
            switch(GetRandomValue(0,3)){
                case 0: ok=SpotIn(cx-110,cz-125,cx-85,cz+125,&p); break; case 1: ok=SpotIn(cx+85,cz-125,cx+110,cz+125,&p); break;
                case 2: ok=SpotIn(cx-125,cz-110,cx+125,cz-85,&p); break; default: ok=SpotIn(cx-125,cz+85,cx+125,cz+110,&p); break; }
            if(ok) SpawnEnemy(PickType(),p,false,false); placed++;
        }
    } else {
        for(int k=0;k<n;k++){
            Vector3 p; int g=k%4; float cx=BASE.center.x,cz=BASE.center.z; bool ok;
            if(g==0) ok=SpotIn(cx-20,BASE.z0+155,cx+20,BASE.z0+165,&p); else if(g==1) ok=SpotIn(cx-20,BASE.z0+5,cx+20,BASE.z0+15,&p);
            else if(g==2) ok=SpotIn(BASE.x0+5,cz-20,BASE.x0+15,cz+20,&p); else ok=SpotIn(BASE.x0+155,cz-20,BASE.x0+165,cz+20,&p);
            int type=PickType(); if(last&&k==n-1&&M.idx>=4) type=EN_COMMANDER;
            if(ok) SpawnEnemy(type,p,true,false);
        }
    }
    M.round=r+1; M.spawned+=n;
}
static void SpawnPickups(void){
    memset(PK,0,sizeof(PK)); int counts[4]={40,18,14,3};
    for(int t=0;t<4;t++) for(int k=0;k<counts[t];k++){
        Vector3 p;
        if(t==3){ if(!SpotIn(BASE.center.x-120,BASE.center.z-120,BASE.center.x+120,BASE.center.z+120,&p)) continue; }
        else if(!SpotIn(20,20,WORLD_SZ-20,WORLD_SZ-20,&p)) continue;
        for(int i=0;i<MAX_PICKUP;i++) if(!PK[i].active){ PK[i]=(Pickup){p,t,true,Rnd01()*6}; break; }
    }
    { static const int ex[5][2]={{4,12},{5,8},{6,6},{7,10},{8,10}};
      for(int t=0;t<5;t++) for(int k=0;k<ex[t][1];k++){ Vector3 q; if(SpotIn(20,20,WORLD_SZ-20,WORLD_SZ-20,&q)) SpawnPickup(q,ex[t][0],(ex[t][0]==4)?GetRandomValue(1,NUM_WEAPONS-1):0,0); } }
    Vector3 p; for(int k=0;k<5;k++) if(SpotIn(BASE.x0+20,BASE.z0+20,BASE.x0+150,BASE.z0+150,&p)){
        int type=(k==0||k==3)?0:(k==1)?2:(k==4)?8:1; for(int i=0;i<MAX_PICKUP;i++) if(!PK[i].active){ PK[i]=(Pickup){p,type,true,0}; break; } }
}
const char *MissionDistrict(int idx){
    int vr[160],vc[160],nv=0;
    for(int r=0;r<BLOCKS;r++) for(int c=0;c<BLOCKS;c++) if(BlockValidForBase(r,c)){ vr[nv]=r; vc[nv]=c; nv++; }
    if(idx<0) idx=0; int bi=(idx*7+3)%nv; return DistrictAt(vc[bi]*BLOCK_SZ+100,vr[bi]*BLOCK_SZ+100);
}
void GameInit(void){ memset(&G,0,sizeof(G)); memset(&IN,0,sizeof(IN)); G.screen=SCR_SPLASH; M.cacheAlive=false; CAM=(Camera3D){V3(800,150,800),V3(800,0,400),V3(0,1,0),60,CAMERA_PERSPECTIVE}; }

void GameStartMission(int idx){
    bool fr=(idx<0); int mi=fr?0:idx;
    memset(&M,0,sizeof(M)); M.idx=idx; M.free=fr; M.total=fr?0:MissionEnemyTotal(mi); M.rounds=fr?0:MissionRounds(mi); M.cacheHP=300; M.cacheAlive=!fr;
    M.par=240.0f+M.total*16.0f;
    snprintf(M.name,sizeof(M.name),"%s",fr?"FREE ROAM":MISSION_NAMES[mi]);
    int vr[160],vc[160],nv=0;
    for(int r=0;r<BLOCKS;r++) for(int c=0;c<BLOCKS;c++) if(BlockValidForBase(r,c)){ vr[nv]=r; vc[nv]=c; nv++; }
    int bi=(mi*7+3)%nv; int br=vr[bi],bc=vc[bi];
    WorldGenerate(br,bc);
    float bx=bc*BLOCK_SZ+100, bz=br*BLOCK_SZ+100;
    int sx[160],sz[160],ns=0;
    for(int r=1;r<BLOCKS;r++) for(int c=1;c<BLOCKS;c++){ float d=hypotf(c*200-bx,r*200-bz); if(fr?(c>=3&&c<=9&&r>=3&&r<=9):(d>=650&&d<=1500)){ sx[ns]=c; sz[ns]=r; ns++; } }
    int si=fr?GetRandomValue(0,ns-1):(mi*5)%ns; M.start=V3(sx[si]*200.0f,0,sz[si]*200.0f);
    snprintf(M.district,sizeof(M.district),"%s",DistrictAt(bx,bz));
    memset(EN,0,sizeof(EN)); memset(GR,0,sizeof(GR)); memset(C4S,0,sizeof(C4S)); memset(PT,0,sizeof(PT)); memset(TR,0,sizeof(TR));
    const CharDef *ch=&CHARS[SV.selChar]; int rank=CharRank(SV.completed);
    memset(&P,0,sizeof(P)); P.rank=rank; P.maxHp=100.0f*ch->hpMul*(1.0f+0.10f*(rank-1)); P.hp=P.maxHp; P.alive=true; P.pos=M.start;
    P.wid[0]=ch->w1; P.wid[1]=ch->w2; for(int s=0;s<2;s++){ P.mag[s]=WEAPONS[P.wid[s]].mag; P.res[s]=WEAPONS[P.wid[s]].reserve; }
    P.gren=ch->gren; P.bombs=ch->bombs;
    G.camYaw=fr?RndR(0,2*PI):atan2f(BASE.center.x-M.start.x,BASE.center.z-M.start.z); G.camPitch=0.12f; P.yaw=G.camYaw;
    G.camDist=4.4f; G.fovCur=SV.st.fov; G.shake=0; G.hurtT=0; G.flashWhite=0; G.mapBig=false; G.completeT=0; G.bannerT=0; G.warnGren=0;
    memset(G.feedT,0,sizeof(G.feedT));
    SpawnPickups(); if(!fr) SpawnRound(0); else M.stage=1;
    CityReset();
    NavFlow(P.pos); navTimer=0;
    if(fr){ GameBanner("FREE ROAM - EXPLORE DHAKA",(Color){255,255,255,255},4.0f); GameFeed("PRESS F NEAR A VEHICLE TO DRIVE",(Color){255,200,80,255}); }
    else { GameBanner(TextFormat("MISSION %d: %s",mi+1,M.name),(Color){255,255,255,255},4.0f); GameFeed("REACH THE ENEMY BASE",(Color){255,200,80,255}); }
    G.screen=SCR_GAME; G.playClock=0;
}
bool GameObjective(Vector3 *pos,char *label,int n){
    if(M.free) return false;
    if(M.stage==2||(M.stage==1&&GameAlive()==0&&M.round>=M.rounds)){ *pos=BASE.cache; snprintf(label,n,"WEAPONS CACHE"); return M.cacheAlive; }
    if(M.stage==0){ *pos=BASE.center; snprintf(label,n,"ENEMY BASE"); return true; }
    float best=1e9f; int bi=-1;
    for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active&&!EN[i].dead&&!EN[i].cop){ float d=V3DistXZ(EN[i].pos,P.pos); if(d<best){best=d;bi=i;} }
    if(bi>=0){ *pos=V3(EN[bi].pos.x,EN[bi].pos.y+1.0f,EN[bi].pos.z); snprintf(label,n,"HOSTILE"); return true; }
    *pos=BASE.center; snprintf(label,n,"ENEMY BASE"); return true;
}
void GameEndMission(bool won){
    G.rKills=P.kills; G.rCiv=P.civKills; G.rHeads=P.heads; G.rScore=P.score+(won?500+(int)fmaxf(0,M.par-M.time)*2:0); G.rTime=M.time; G.rNewChar=-1; G.rRank=false; G.rStars=0;
    SV.totalKills+=P.kills; SV.totalHeadshots+=P.heads; SV.totalScore+=G.rScore; SV.playTime+=M.time;
    if(won){
        int before=SV.completed, after=(M.idx+1>before)?M.idx+1:before;
        G.rStars=1+(P.dmgTaken<P.maxHp*0.8f?1:0)+(M.time<M.par?1:0); if(P.civKills>=3&&G.rStars>1) G.rStars--;
        if(G.rStars>SV.stars[M.idx]) SV.stars[M.idx]=(unsigned char)G.rStars;
        for(int i=0;i<NUM_CHARS;i++) if(CHARS[i].need>before&&CHARS[i].need<=after){ G.rNewChar=i; break; }
        G.rRank=CharRank(after)>CharRank(before); SV.completed=after;
        G.screen=SCR_COMPLETE;
    } else G.screen=SCR_OVER;
    SaveWrite();
}
static void MissionUpdate(float dt){
    M.time+=dt; c4Msg-=dt;
    if(M.free) return;
    int alive=GameAlive();
    if(M.stage==0){
        if(V3DistXZ(P.pos,BASE.center)<80||(alive==0&&M.round>=1)){ M.stage=1; GameBanner("ENEMY BASE REACHED - ELIMINATE ALL HOSTILES",(Color){255,90,70,255},3.5f); }
    }
    if(M.stage==1&&alive==0){
        if(M.round<M.rounds){
            if(!M.roundPending){ M.roundPending=true; M.roundDelay=4.5f; GameBanner(TextFormat("ROUND %d/%d CLEARED",M.round,M.rounds),(Color){120,255,150,255},3.0f); }
            M.roundDelay-=dt;
            if(M.roundDelay<=0){ M.roundPending=false; SpawnRound(M.round); GameBanner(TextFormat("ROUND %d/%d - REINFORCEMENTS INBOUND",M.round,M.rounds),(Color){255,120,70,255},3.5f); GameFeed("HOSTILES AT THE GATES",(Color){255,120,70,255}); }
        } else {
            M.stage=2; GameBanner("DESTROY THE WEAPONS CACHE WITH C4 OR GRENADES",(Color){255,170,60,255},4.5f);
            if(P.bombs<1){ P.bombs++; GameFeed("SUPPLY: +1 C4",(Color){255,160,90,255}); } if(P.gren<3){ P.gren=3; GameFeed("SUPPLY: GRENADES",(Color){200,230,140,255}); }
        }
    }
    if(M.stage==2&&M.cacheHP<=0){
        M.stage=3; M.cacheAlive=false; for(int i=0;i<nObs;i++) if(OBS[i].kind==OB_CACHE) OBS[i].active=false;
        Explode(BASE.cache,18,0,true); Explode(V3Add(BASE.cache,V3(3,0,2)),12,0,true);
        GameBanner("WEAPONS CACHE DESTROYED - MISSION COMPLETE",(Color){120,255,150,255},4.0f); G.completeT=3.2f;
    }
    if(M.stage==3){ G.completeT-=dt; if(G.completeT<=0&&P.alive) GameEndMission(true); }
}

/* ---------- camera & per-frame ---------- */
static Vector3 drvCam; static bool drvInit=false; static float lookIdle=0;
static void UpdateGameCamera(float dt){
    Vector3 pos,target; Vector3 f,r;
    if(P.inVeh){
        const Vehicle *v=&VEH[P.vehIdx]; float sp=fabsf(v->speed);
        if(fabsf(IN.look_x)>0.0008f||fabsf(IN.look_y)>0.0008f) lookIdle=0; else lookIdle+=dt;
        if(lookIdle>0.8f){
            float rate=Clampf(2.0f+sp*0.16f,2.0f,5.5f);
            G.camYaw+=AngleDiff(G.camYaw,v->yaw)*(1.0f-expf(-rate*dt));
            G.camPitch=Lerpf(G.camPitch,0.17f+Clampf(sp/80.0f,0,0.08f),1.0f-expf(-2.5f*dt));
        }
        float fov=SV.st.fov*(1.0f+Clampf(sp/70.0f,0,0.22f)); G.fovCur=Lerpf(G.fovCur,fov,1.0f-expf(-4.0f*dt));
        G.camDist=Lerpf(G.camDist,((v->type==3)?14.0f:8.5f)+sp*0.10f,1.0f-expf(-3.0f*dt));
        Vector3 hdg=YawDir(v->yaw); target=V3(P.pos.x+hdg.x*v->speed*0.10f,(v->type==3)?2.8f:1.8f,P.pos.z+hdg.z*v->speed*0.10f);
        f=CamForward(); Vector3 d=V3Mul(f,-1.0f); float len=G.camDist;
        float t=WorldRayT(target,d,len+0.4f,NULL); if(t<len+0.4f) len=fmaxf(1.5f,t-0.4f);
        Vector3 desired=V3Add(target,V3Mul(d,len)); if(desired.y<0.8f) desired.y=0.8f;
        if(!drvInit){ drvCam=desired; drvInit=true; } else { float k=1.0f-expf(-dt*10.0f); drvCam=V3(Lerpf(drvCam.x,desired.x,k),Lerpf(drvCam.y,desired.y,k),Lerpf(drvCam.z,desired.z,k)); }
        pos=drvCam;
    } else {
        drvInit=false; lookIdle=0;
        float tf=P.aiming?((WEAPONS[P.wid[P.cw]].sfx==SFX_SNIPER)?0.38f:0.72f):1.0f;
        G.fovCur=Lerpf(G.fovCur,SV.st.fov*tf,fminf(1,dt*10));
        float wantD=P.aiming?((tf<0.5f)?1.6f:2.5f):4.4f; G.camDist=Lerpf(G.camDist,wantD,fminf(1,dt*10));
        f=CamForward(); r=YawRight(G.camYaw);
        Vector3 tgt=V3Add(V3(P.pos.x,P.pos.y+1.55f,P.pos.z),V3Mul(r,P.aiming?0.55f:0.85f));
        Vector3 d=V3Mul(f,-1.0f); float len=G.camDist;
        float t=WorldRayT(tgt,d,len+0.3f,NULL); if(t<len+0.3f) len=fmaxf(0.7f,t-0.3f);
        pos=V3Add(tgt,V3Mul(d,len)); if(pos.y<0.4f) pos.y=0.4f; target=V3Add(pos,V3Mul(f,20.0f));
    }
    if(G.shake>0){ pos=V3Add(pos,V3(RndR(-1,1)*G.shake*0.25f,RndR(-1,1)*G.shake*0.25f,RndR(-1,1)*G.shake*0.25f)); G.shake=fmaxf(0,G.shake-dt*2.5f); }
    CAM.position=pos; CAM.target=target; CAM.up=V3(0,1,0); CAM.fovy=G.fovCur; CAM.projection=CAMERA_PERSPECTIVE;
}
void GameUpdate(float dt){
    if(dt>0.05f) dt=0.05f;
    G.playClock+=dt;
    if(P.alive) PlayerUpdate(dt);
    navTimer-=dt; if(navTimer<=0){ NavFlow(P.pos); navTimer=0.35f; }
    if(M.stage!=3||P.alive) for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active) EnemyUpdate(&EN[i],i,dt);
    { float mx=0; bool combat=false,search=false;
      for(int i=0;i<MAX_ENEMY;i++){ const Enemy *e=&EN[i]; if(!e->active||e->dead) continue;
          if(e->state==1){ if(e->canSee) combat=true; else search=true; } else if(e->spot>mx) mx=e->spot; }
      float tg=combat?1.0f:(search?0.6f:Clampf(mx,0,0.95f)); G.detect=Lerpf(G.detect,tg,fminf(1,dt*6.0f));
      int ns=combat?2:((search||mx>0.3f)?1:0); if(ns==2&&G.detState<2) GameFeed("DETECTED!",(Color){255,70,70,255}); G.detState=ns; }
    UpdateGrenades(dt); UpdateC4(dt); ExplodeBarrelUpdate(dt);
    CityUpdate(dt);
    MissionUpdate(dt);
    for(int i=0;i<MAX_PART;i++){ Part *p=&PT[i]; if(!p->active) continue; p->life-=dt; if(p->life<=0){ p->active=false; continue; }
        p->p=V3Add(p->p,V3Mul(p->v,dt));
        switch(p->kind){ case PF_DEBRIS: p->v.y-=16*dt; if(p->p.y<0.05f){ p->p.y=0.05f; p->v=V3Mul(p->v,0.3f); p->v.y=0; } break;
            case PF_SPARK: p->v.y-=10*dt; break; case PF_FIRE: p->v=V3Mul(p->v,1.0f-dt*2.0f); p->v.y+=2.0f*dt; break;
            case PF_SMOKE: p->v=V3Mul(p->v,1.0f-dt*0.8f); break; default: break; } }
    for(int i=0;i<MAX_TRACER;i++) if(TR[i].life>0) TR[i].life-=dt;
    for(int i=0;i<5;i++) if(G.feedT[i]>0) G.feedT[i]-=dt;
    if(G.bannerT>0) G.bannerT-=dt; if(G.hurtT>0) G.hurtT-=dt; if(G.hitT>0) G.hitT-=dt; if(G.flashT>0) G.flashT-=dt;
    G.flashWhite=fmaxf(0,G.flashWhite-dt*1.2f); if(G.warnGren>0) G.warnGren-=dt;
    UpdateGameCamera(dt);
    gListenPos=V3(P.pos.x,P.pos.y+1.5f,P.pos.z); gListenYaw=G.camYaw;
}

/* ---------- 3D drawing ---------- */
static void Shadow(Vector3 p,float s){ if(SV.st.blobShadow) DrawFlat(p.x,p.y+0.14f,p.z,s,s,(Color){0,0,0,70}); }
void GameDraw3D(void){
    Vector3 fwd=V3Norm(V3(CAM.target.x-CAM.position.x,0,CAM.target.z-CAM.position.z));
    WorldDraw(CAM.position,fwd,SV.st.drawDist,(float)GetTime());
    float tt=(float)GetTime();
    for(int i=0;i<MAX_PICKUP;i++){ const Pickup *k=&PK[i]; if(!k->active||V3DistXZ(k->pos,CAM.position)>240) continue;
        Vector3 c=V3(k->pos.x,0.75f+sinf(tt*2+k->t)*0.12f,k->pos.z); rlPushMatrix(); rlTranslatef(c.x,c.y,c.z); rlRotatef(tt*80,0,1,0);
        static const Color col[9]={{235,235,235,255},{110,150,70,255},{70,90,50,255},{120,110,60,255},{60,60,66,255},{70,110,190,255},{170,90,210,255},{210,170,50,255},{245,245,245,255}};
        if(k->type==4) DrawWeaponWorld(k->wid,V3(0,0,0),PI/2,1.5f);
        else if(k->type==6) BoxC(V3(0,0,0),V3(0.18f,0.18f,0.55f),col[6]);
        else if(k->type==7) BoxC(V3(0,0,0),V3(0.6f,0.4f,0.4f),col[7]);
        else if(k->type==8) BoxC(V3(0,0,0),V3(0.8f,0.55f,0.55f),col[8]);
        else BoxC(V3(0,0,0),V3(0.55f,0.4f,0.4f),col[k->type]);
        if(k->type==5) BoxC(V3(0,0.05f,0.22f),V3(0.3f,0.2f,0.04f),(Color){220,220,230,255});
        if(k->type==0){ BoxC(V3(0,0.05f,0.21f),V3(0.34f,0.1f,0.02f),(Color){210,40,40,255}); BoxC(V3(0,0.05f,0.21f),V3(0.1f,0.32f,0.02f),(Color){210,40,40,255}); }
        if(k->type==8){ BoxC(V3(0,0.0f,0.29f),V3(0.5f,0.14f,0.02f),(Color){210,40,40,255}); BoxC(V3(0,0.0f,0.29f),V3(0.14f,0.46f,0.02f),(Color){210,40,40,255}); BoxC(V3(0,0.31f,0),V3(0.3f,0.06f,0.1f),(Color){60,60,66,255}); }
        if(k->type==3) BoxC(V3(0,0.24f,0),V3(0.1f,0.08f,0.1f),(Color){255,40,40,255});
        rlPopMatrix();
        static const Color gl[9]={{120,255,150,120},{255,220,100,90},{180,230,120,90},{255,150,80,90},{255,210,90,110},{110,170,255,90},{220,140,255,100},{255,220,80,100},{80,255,160,140}};
        BoxC(V3(k->pos.x,2.0f,k->pos.z),V3(0.12f,4.0f,0.12f),gl[k->type]);
        Shadow(V3(k->pos.x,0,k->pos.z),0.8f);
    }
    for(int i=0;i<MAX_C4;i++) if(C4S[i].active){ Vector3 p=C4S[i].pos; BoxC(V3(p.x,0.1f,p.z),V3(0.5f,0.2f,0.35f),(Color){90,96,70,255});
        bool on=fmodf(tt*2,1.0f)<0.5f; BoxC(V3(p.x,0.23f,p.z),V3(0.1f,0.06f,0.1f),on?(Color){255,40,40,255}:(Color){80,10,10,255}); }
    for(int i=0;i<MAX_GREN;i++) if(GR[i].active){ DrawSphereEx(GR[i].pos,0.12f,6,6,GR[i].enemy?(Color){120,60,50,255}:(Color){60,80,50,255}); if(fmodf(tt*6,1.0f)<0.4f) DrawSphereEx(V3Add(GR[i].pos,V3(0,0.12f,0)),0.04f,4,4,RED); }
    for(int i=0;i<MAX_ENEMY;i++){ const Enemy *e=&EN[i]; if(!e->active) continue; if(V3Dist(e->pos,CAM.position)>(SV.st.drawDist==0?160:260)) continue;
        float pitch=0; if(e->state==1&&!e->dead) pitch=atan2f((P.pos.y+1.2f)-(e->pos.y+1.5f),fmaxf(1,V3DistXZ(e->pos,P.pos)));
        if(!e->dead) Shadow(e->pos,1.0f);
        DrawHuman(&e->look,e->pos,e->yaw,e->walk,e->speedN,pitch,e->wid,e->state==1,e->dead?e->fall:0,e->flash,0);
    }
    CityDraw(CAM.position,fwd);
    if(P.alive&&!P.inVeh){ Shadow(P.pos,1.0f); DrawHuman(&CHARS[SV.selChar].look,P.pos,P.yaw,P.walk,P.speedN,P.pitch,P.wid[P.cw],P.aiming||P.fireT>-0.4f,0,0,P.rank); }
    else if(!P.inVeh) DrawHuman(&CHARS[SV.selChar].look,P.pos,P.yaw,0,0,0,P.wid[P.cw],false,1.0f,0,P.rank);
    /* objective beacons */
    if(M.cacheAlive&&!M.free){
        if(M.stage<2) BoxC(V3(BASE.center.x,60,BASE.center.z),V3(1.2f,120,1.2f),(Color){255,60,50,70});
        else BoxC(V3(BASE.cache.x,60,BASE.cache.z),V3(1.2f,120,1.2f),(Color){255,160,40,90});
    }
    for(int i=0;i<MAX_TRACER;i++) if(TR[i].life>0){
        Vector3 d=V3Sub(TR[i].b,TR[i].a); float len=V3Len(d); if(len<0.5f) continue; Vector3 n=V3Mul(d,1.0f/len);
        float prog=1.0f-TR[i].life/0.09f, head=len*prog, tail=fmaxf(0,head-9.0f);
        DrawCylinderEx(V3Add(TR[i].a,V3Mul(n,tail)),V3Add(TR[i].a,V3Mul(n,head)),0.015f,0.03f,4,TR[i].c); }
    for(int i=0;i<MAX_PART;i++){ const Part *p=&PT[i]; if(!p->active) continue; float t=p->life/p->maxLife;
        switch(p->kind){
            case PF_FIRE: { Color c=p->c; c.a=(unsigned char)(255*t); DrawSphereEx(p->p,p->size*(0.4f+0.6f*t),5,5,c); } break;
            case PF_SMOKE: { Color c=p->c; c.a=(unsigned char)(c.a*t*0.8f); DrawSphereEx(p->p,p->size*(1.4f-0.6f*t),5,5,c); } break;
            case PF_DEBRIS: BoxC(p->p,V3(p->size,p->size,p->size),p->c); break;
            case PF_SPARK: BoxC(p->p,V3(p->size,p->size,p->size),p->c); break;
            case PF_FLASH: DrawSphereEx(p->p,p->size,5,5,p->c); break; } }
    if(G.flashT>0){ float s=(0.3f-G.flashT)/0.3f; DrawSphereEx(G.flashPos,2+s*10,8,8,(Color){255,200,120,(unsigned char)(160*(1-s))}); }
}
