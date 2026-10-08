/* City life: pedestrians, vendors, moving traffic with signals, carjacking and driving */
#include "game.h"
#define CC(r,g,b) ((Color){r,g,b,255})

Citizen CIT[MAX_CIT]; Vehicle VEH[MAX_VEH];
static float cityTime=0,spawnT=0; static Look pullerLook;
bool gPoliceSeen=false; int gWanted=0; float gHeat=0; static float dispatchT=0,runHitCd=0;

static const float VCRUISE[4]={4.5f,10.5f,13.0f,9.5f}, VMAX[4]={9,22,30,19}, VACC[4]={4,9,11,6};
static const float VHL[4]={1.35f,1.45f,2.2f,5.2f}, VHW[4]={0.7f,0.8f,0.95f,1.3f}, VHP[4]={120,170,220,450};

int SignalState(int axis){            /* axis 0 = traffic along X, axis 1 = along Z */
    float t=fmodf(cityTime,24.0f);
    if(t<10) return axis==0; if(t<12) return 0; if(t<22) return axis==1; return 0;
}
static float HeadYaw(int axis,int dir){ return axis==0?(dir>0?PI/2:-PI/2):(dir>0?0.0f:PI); }
static int CitTarget(void){ static const int t[3]={36,64,100}; return t[SV.st.drawDist]; }
static int VehTarget(void){ static const int t[3]={14,26,38}; return t[SV.st.drawDist]; }
static bool NearBase(Vector3 p){ return !M.free&&V3DistXZ(p,BASE.center)<140; }

/* ---------- citizens ---------- */
static Look MakeCitizen(void){
    static const Color skins[5]={{150,104,72,255},{171,120,85,255},{110,72,50,255},{196,148,110,255},{125,84,58,255}};
    static const Color shM[7]={CC(240,240,236),CC(120,160,210),CC(90,130,100),CC(200,170,120),CC(150,150,160),CC(220,200,90),CC(170,90,80)};
    static const Color shF[6]={CC(200,60,90),CC(60,140,170),CC(230,170,40),CC(90,160,110),CC(170,80,160),CC(230,120,60)};
    static const Color lungi[3]={CC(70,100,160),CC(150,70,70),CC(60,120,90)};
    Look l; memset(&l,0,sizeof(l)); l.skin=skins[GetRandomValue(0,4)]; l.hair=CC(22,16,14);
    if(GetRandomValue(0,99)<38){
        int k=GetRandomValue(0,5); l.shirt=shF[k]; l.pants=ColMul(shF[k],0.8f); l.skirt=true; l.skirtCol=ColMul(shF[(k+3)%6],0.95f); l.hairType=2;
        if(GetRandomValue(0,1)){ l.hatType=7; l.hat=shF[(k+2)%6]; }
    } else {
        l.shirt=shM[GetRandomValue(0,6)]; l.hairType=1; l.beard=GetRandomValue(0,3)==0;
        if(GetRandomValue(0,2)==0){ l.skirt=true; l.skirtCol=lungi[GetRandomValue(0,2)]; l.pants=l.skirtCol; }
        else l.pants=GetRandomValue(0,1)?CC(40,44,60):CC(120,110,90);
        if(GetRandomValue(0,99)<30) l.hatType=8;
    }
    l.vest=l.shirt; return l;
}
static int FreeCit(void){ for(int i=0;i<MAX_CIT;i++) if(!CIT[i].active) return i; return -1; }
static void ReleaseStall(Citizen *c){ if(c->stall>=0&&c->stall<nStall) STALLS[c->stall].taken=false; c->stall=-1; }
static void Reattach(Citizen *c){
    float bestD=1e9f; int bAxis=0; float bLine=0;
    for(int ax=0;ax<2;ax++){ float v=(ax==0)?c->pos.z:c->pos.x; int k=(int)roundf(v/BLOCK_SZ);
        for(int kk=k-1;kk<=k+1;kk++) for(int s=-1;s<=1;s+=2){ float ln=kk*BLOCK_SZ+s*16.0f; if(ln<16||ln>WORLD_SZ-16) continue;
            float d=fabsf(v-ln); if(d<bestD){ bestD=d; bAxis=ax; bLine=ln; } } }
    c->axis=bAxis; c->line=bLine; float hx=sinf(c->yaw),hz=cosf(c->yaw); c->dir=((bAxis==0?hx:hz)>=0)?1:-1; c->vendor=false; ReleaseStall(c);
}
static bool SpawnCitizen(float minD,float maxD){
    int slot=FreeCit(); if(slot<0) return false;
    Vector3 pp=P.pos;
    if(GetRandomValue(0,99)<14&&nStall>0){
        int pick=-1,seen=0;
        for(int i=0;i<nStall;i++){ if(STALLS[i].taken) continue; float d=V3DistXZ(STALLS[i].pos,pp); if(d<minD||d>maxD) continue; seen++; if(GetRandomValue(0,seen-1)==0) pick=i; }
        if(pick>=0){
            Vector3 v=V3Add(STALLS[pick].pos,V3Mul(YawDir(STALLS[pick].yaw),1.8f));
            if(!NearBase(v)&&!IsWaterAt(v.x,v.z)){
                Citizen *c=&CIT[slot]; memset(c,0,sizeof(*c)); c->active=true; c->vendor=true; c->stall=pick; c->pos=v; c->pos.y=0; c->yaw=STALLS[pick].yaw; c->hp=30; c->look=MakeCitizen(); c->look.skirt=false;
                STALLS[pick].taken=true; return true; } }
    }
    for(int a=0;a<10;a++){
        int axis=GetRandomValue(0,1); float perp=((axis==0)?pp.z:pp.x)+RndR(-maxD,maxD);
        int k=(int)roundf(perp/BLOCK_SZ); if(k<0)k=0; if(k>BLOCKS)k=BLOCKS;
        int side=(k==0)?1:(k==BLOCKS)?-1:(GetRandomValue(0,1)?1:-1); float line=k*BLOCK_SZ+side*16.0f;
        float t=Clampf(((axis==0)?pp.x:pp.z)+RndR(-maxD,maxD),20,WORLD_SZ-20);
        Vector3 pos=(axis==0)?V3(t,0,line):V3(line,0,t); float d=V3DistXZ(pos,pp);
        if(d<minD||d>maxD||NearBase(pos)||IsWaterAt(pos.x,pos.z)||!WorldFree(pos.x,pos.z,0.4f)) continue;
        Citizen *c=&CIT[slot]; memset(c,0,sizeof(*c)); c->active=true; c->pos=pos; c->axis=axis; c->dir=GetRandomValue(0,1)?1:-1; c->line=line;
        c->hp=30; c->speed=RndR(1.1f,1.8f); c->look=MakeCitizen(); c->yaw=HeadYaw(axis,c->dir); c->stall=-1; c->turnCd=RndR(0,2); return true;
    }
    return false;
}
static void SpawnFleeing(Vector3 at){
    int slot=FreeCit(); if(slot<0) return; Citizen *c=&CIT[slot]; memset(c,0,sizeof(*c));
    c->active=true; c->pos=at; c->pos.y=0; c->hp=30; c->look=MakeCitizen(); c->stall=-1; c->flee=8; c->threat=P.pos; c->yaw=RndR(0,2*PI); c->speed=1.5f;
}
void CityAlarm(Vector3 at,float radius){
    for(int i=0;i<MAX_CIT;i++){ Citizen *c=&CIT[i]; if(!c->active||c->dead) continue;
        if(V3DistXZ(c->pos,at)<radius){ c->flee=RndR(6,10); c->threat=at; } }
}
static void CitizenUpdate(Citizen *c,float dt){
    if(V3DistXZ(c->pos,P.pos)>300){ ReleaseStall(c); c->active=false; return; }
    if(c->dead){ c->deadT+=dt; c->fall=fminf(1,c->fall+dt*3); if(c->deadT>14) c->active=false; return; }
    c->turnCd-=dt;
    if(c->flee>0){
        c->flee-=dt; Vector3 away=V3Norm(V3(c->pos.x-c->threat.x,0,c->pos.z-c->threat.z)); if(V3Len(away)<0.1f) away=V3(1,0,0);
        c->yaw+=AngleDiff(c->yaw,atan2f(away.x,away.z))*fminf(1,dt*8);
        Vector3 old=c->pos; c->pos=V3Add(c->pos,V3Mul(YawDir(c->yaw),6.2f*dt)); MoveCollide(&c->pos,0.35f,NULL);
        float mv=V3DistXZ(old,c->pos)/fmaxf(dt,1e-4f);
        if(mv<2.0f){ c->stuck+=dt; if(c->stuck>0.4f){ c->yaw+=RndR(-2.2f,2.2f); c->stuck=0; } } 
        c->speedN=1.25f; c->walk+=dt*11;
        if(c->flee<=0){ c->speed=RndR(1.1f,1.8f); Reattach(c); }
        return;
    }
    if(c->vendor){ c->speedN=Lerpf(c->speedN,0,fminf(1,dt*6)); return; }
    float adv=c->speed*dt; Vector3 old=c->pos;
    float *t=(c->axis==0)?&c->pos.x:&c->pos.z, *lat=(c->axis==0)?&c->pos.z:&c->pos.x;
    *t+=c->dir*adv; *lat+=(c->line-*lat)*fminf(1,dt*3);
    MoveCollide(&c->pos,0.35f,NULL);
    float prog=V3DistXZ(old,c->pos);
    if(prog<adv*0.25f){ c->stuck+=dt; if(c->stuck>1.0f){ c->dir=-c->dir; c->stuck=0; } } else c->stuck=fmaxf(0,c->stuck-dt);
    if(*t<18) c->dir=1; if(*t>WORLD_SZ-18) c->dir=-1;
    if(c->turnCd<=0){
        int kk=(int)roundf(*t/BLOCK_SZ);
        for(int s=-1;s<=1;s+=2){ float cl=kk*BLOCK_SZ+s*16.0f; if(cl<16||cl>WORLD_SZ-16) continue;
            if(fabsf(*t-cl)<0.4f){ c->turnCd=2.0f;
                if(Rnd01()<0.35f){ c->axis=1-c->axis; c->line=cl; c->dir=GetRandomValue(0,1)?1:-1; }
                break; } }
    }
    c->yaw+=AngleDiff(c->yaw,HeadYaw(c->axis,c->dir))*fminf(1,dt*8);
    c->speedN=1.0f; c->walk+=dt*c->speed*6.0f;
}

/* ---------- traffic ---------- */
static int PickVehType(Vector3 p){
    float roll=Rnd01(); const char *d=DistrictAt(p.x,p.z);
    if(strcmp(d,"OLD DHAKA")==0) return (roll<0.6f)?0:(roll<0.8f)?1:(roll<0.9f)?2:3;
    if(strcmp(d,"GULSHAN")==0||strcmp(d,"MOTIJHEEL")==0||strcmp(d,"UTTARA")==0) return (roll<0.15f)?0:(roll<0.3f)?1:(roll<0.85f)?2:3;
    return (roll<0.3f)?0:(roll<0.5f)?1:(roll<0.8f)?2:3;
}
static Color VehColor(int type){
    static const Color cols[6]={CC(210,50,45),CC(40,110,190),CC(240,200,50),CC(60,160,90),CC(230,230,230),CC(150,60,160)};
    if(type==1) return CC(40,150,80); if(type==3) return CC(210,70,40); return cols[GetRandomValue(0,5)];
}
static bool SpawnTraffic(float minD,float maxD){
    int slot=-1; for(int i=0;i<MAX_VEH;i++) if(!VEH[i].active){ slot=i; break; } if(slot<0) return false;
    Vector3 pp=P.pos;
    for(int a=0;a<12;a++){
        int axis=GetRandomValue(0,1), dir=GetRandomValue(0,1)?1:-1; float perp=((axis==0)?pp.z:pp.x)+RndR(-maxD,maxD);
        int k=(int)roundf(perp/BLOCK_SZ); if(k<1)k=1; if(k>BLOCKS-1)k=BLOCKS-1;
        float t=Clampf(((axis==0)?pp.x:pp.z)+RndR(-maxD,maxD),40,WORLD_SZ-40);
        float lane=(axis==1)?k*BLOCK_SZ+5.0f*dir:k*BLOCK_SZ-5.0f*dir;
        Vector3 pos=(axis==0)?V3(t,0,lane):V3(lane,0,t); float d=V3DistXZ(pos,pp);
        if(d<minD||d>maxD) continue;
        Vector3 h=YawDir(HeadYaw(axis,dir));
        if(IsWaterAt(pos.x,pos.z)||IsWaterAt(pos.x+h.x*40,pos.z+h.z*40)||IsWaterAt(pos.x-h.x*40,pos.z-h.z*40)) continue;
        bool clash=false; for(int j=0;j<MAX_VEH;j++) if(VEH[j].active&&V3DistXZ(VEH[j].pos,pos)<18) clash=true; if(clash) continue;
        Vehicle *v=&VEH[slot]; memset(v,0,sizeof(*v)); v->active=true; v->type=PickVehType(pos); v->axis=axis; v->dir=dir; v->lineIdx=k; v->pos=pos; v->yaw=HeadYaw(axis,dir);
        v->speed=VCRUISE[v->type]*0.8f; v->hp=v->maxHp=VHP[v->type]; v->col=VehColor(v->type); v->turnCd=RndR(0,3); return true;
    }
    return false;
}
static void VehicleDestroy(int idx,bool byPlayer){
    Vehicle *v=&VEH[idx]; if(v->wreck) return;
    v->wreck=true; v->speed=0; v->col=CC(36,34,32); v->hp=0; v->parked=true; bool was=v->player; v->player=false;
    if(was){ P.inVeh=false; P.pos=V3Add(v->pos,V3Mul(YawRight(v->yaw),VHW[v->type]+2.0f)); P.pos.y=0; MoveCollide(&P.pos,0.42f,NULL); }
    if(v->police&&byPlayer) CityCrime(1,"POLICE VEHICLE DESTROYED");
    GameExplode(V3(v->pos.x,0.8f,v->pos.z),9,120,byPlayer);
}
static void TrafficUpdate(Vehicle *v,int idx,float dt){
    float desired=VCRUISE[v->type]*(0.9f+0.04f*(idx%5)); float limit=desired;
    Vector3 f=YawDir(v->yaw), r=YawRight(v->yaw); float look=10.0f+v->speed*0.9f, hl=VHL[v->type], hw=VHW[v->type];
    for(int j=0;j<MAX_VEH;j++){ if(j==idx||!VEH[j].active) continue; Vector3 d=V3Sub(VEH[j].pos,v->pos); float fd=V3Dot(d,f), ld=V3Dot(d,r);
        float need=hl+VHL[VEH[j].type]+3.5f;
        if(fd>0&&fd<look+need&&fabsf(ld)<hw+VHW[VEH[j].type]+0.8f){ float gap=fd-need; float lim=(gap<=0)?0:fminf(desired,VEH[j].speed+gap*0.6f); if(lim<limit) limit=lim; } }
    for(int j=0;j<MAX_CIT+1;j++){
        Vector3 cp; if(j<MAX_CIT){ if(!CIT[j].active||CIT[j].dead) continue; cp=CIT[j].pos; } else { if(P.inVeh) continue; cp=P.pos; }
        Vector3 d=V3Sub(cp,v->pos); float fd=V3Dot(d,f), ld=V3Dot(d,r);
        if(fd>0&&fd<look*0.9f+hl&&fabsf(ld)<hw+1.1f){ float gap=fd-hl-2.0f; float lim=(gap<=0)?0:fminf(desired,gap*0.7f); if(lim<limit) limit=lim; } }
    { float c=(v->axis==0)?v->pos.x:v->pos.z; int j=(v->dir>0)?(int)floorf(c/BLOCK_SZ)+1:(int)ceilf(c/BLOCK_SZ)-1;
      if(j>=1&&j<=BLOCKS-1){ float dist=(j*BLOCK_SZ-c)*v->dir; float dsl=dist-18.0f-hl;
          if(dist>0&&SignalState(v->axis)==0&&dsl<35&&dsl>-1.0f){ if(dsl<=0.5f) limit=0; else limit=fminf(limit,sqrtf(2*3.5f*dsl)); } } }
    v->braking=limit<v->speed-0.3f;
    if(limit<v->speed) v->speed=fmaxf(limit,v->speed-9.0f*dt); else v->speed=fminf(limit,v->speed+VACC[v->type]*dt);
    float lane=(v->axis==1)?v->lineIdx*BLOCK_SZ+5.0f*v->dir:v->lineIdx*BLOCK_SZ-5.0f*v->dir;
    float lat=(v->axis==1)?v->pos.x:v->pos.z, e=lane-lat, sgn=(v->axis==1)?(float)v->dir:(float)-v->dir;
    float target=HeadYaw(v->axis,v->dir)+sgn*Clampf(e*0.12f,-0.6f,0.6f), mt=2.4f*dt;
    v->yaw+=Clampf(AngleDiff(v->yaw,target),-mt,mt);
    float oldT=(v->axis==0)?v->pos.x:v->pos.z;
    v->pos=V3Add(v->pos,V3Mul(YawDir(v->yaw),v->speed*dt)); v->life+=v->speed*dt*2.0f;
    float newT=(v->axis==0)?v->pos.x:v->pos.z; v->turnCd-=dt;
    int j0=(int)roundf(newT/BLOCK_SZ); float cc=j0*BLOCK_SZ;
    if(v->turnCd<=0&&j0>=1&&j0<=BLOCKS-1&&(oldT-cc)*(newT-cc)<=0&&oldT!=newT){
        v->turnCd=3;
        if(Rnd01()<0.35f){
            int na=1-v->axis, nd=GetRandomValue(0,1)?1:-1;
            Vector3 center=(v->axis==1)?V3(v->lineIdx*BLOCK_SZ,0,j0*BLOCK_SZ):V3(j0*BLOCK_SZ,0,v->lineIdx*BLOCK_SZ);
            Vector3 probe=(na==0)?V3(center.x+nd*60,0,center.z):V3(center.x,0,center.z+nd*60);
            if(probe.x>30&&probe.x<WORLD_SZ-30&&probe.z>30&&probe.z<WORLD_SZ-30&&!IsWaterAt(probe.x,probe.z)){ v->axis=na; v->dir=nd; v->lineIdx=j0; }
        }
    }
    Vector3 ahead=V3Add(v->pos,V3Mul(YawDir(v->yaw),22.0f));
    if(newT<25||newT>WORLD_SZ-25||IsWaterAt(ahead.x,ahead.z)) v->active=false;
}

/* ---------- entering, leaving, driving ---------- */
static int FindEnterable(int *obsIdx){
    int best=-1; float bd=1e9f; *obsIdx=-1;
    for(int i=0;i<MAX_VEH;i++){ const Vehicle *v=&VEH[i]; if(!v->active||v->wreck||v->player) continue;
        float d=V3DistXZ(v->pos,P.pos)-VHL[v->type]; if(d<2.6f&&d<bd){ bd=d; best=i; } }
    for(int i=0;i<nObs;i++){ const Obs *o=&OBS[i]; if(o->kind!=OB_VEH||!o->active) continue;
        float dx=fmaxf(fmaxf(o->mn.x-P.pos.x,0),P.pos.x-o->mx.x), dz=fmaxf(fmaxf(o->mn.z-P.pos.z,0),P.pos.z-o->mx.z), d=sqrtf(dx*dx+dz*dz);
        if(d<2.4f&&d<bd){ bd=d; best=-1; *obsIdx=i; } }
    return best;
}
bool CityNearVehicle(void){ if(P.inVeh) return true; int oi; int b=FindEnterable(&oi); return b>=0||oi>=0; }
void CityUse(void){
    if(P.inVeh){
        Vehicle *v=&VEH[P.vehIdx]; Vector3 side=V3Mul(YawRight(v->yaw),VHW[v->type]+1.2f);
        Vector3 p=V3Add(v->pos,side); if(!WorldFree(p.x,p.z,0.4f)){ p=V3Sub(v->pos,side); if(!WorldFree(p.x,p.z,0.4f)) p=V3Sub(v->pos,V3Mul(YawDir(v->yaw),VHL[v->type]+1.5f)); }
        P.pos=p; P.pos.y=0; MoveCollide(&P.pos,0.42f,NULL); P.inVeh=false; v->player=false; v->parked=true; return;
    }
    int oi; int b=FindEnterable(&oi); bool hijack=false;
    if(b<0&&oi>=0){
        int slot=-1; for(int i=0;i<MAX_VEH;i++) if(!VEH[i].active){ slot=i; break; }
        if(slot<0){ GameFeed("NO FREE VEHICLE SLOT",(Color){255,90,90,255}); return; }
        Obs *o=&OBS[oi]; Vehicle *v=&VEH[slot]; memset(v,0,sizeof(*v)); bool alongZ=o->style&1;
        v->active=true; v->type=o->style>>1; v->axis=alongZ?1:0; v->dir=1; v->pos=V3((o->mn.x+o->mx.x)/2,0,(o->mn.z+o->mx.z)/2); v->yaw=alongZ?0.0f:PI/2;
        v->lineIdx=(int)roundf(((alongZ)?v->pos.x:v->pos.z)/BLOCK_SZ); v->hp=v->maxHp=VHP[v->type]; v->col=o->col; v->parked=true; o->active=false; b=slot;
    } else if(b>=0) hijack=!VEH[b].parked;
    if(b<0) return;
    Vehicle *v=&VEH[b];
    if(hijack){ SpawnFleeing(V3Add(v->pos,V3Mul(YawRight(v->yaw),VHW[v->type]+1.2f))); GameFeed("VEHICLE HIJACKED",(Color){255,200,60,255}); CityAlarm(v->pos,40); }
    else GameFeed("ENTERED VEHICLE",(Color){255,255,255,255});
    v->player=true; v->parked=false; P.inVeh=true; P.vehIdx=b; P.aiming=false; P.pos=v->pos; P.yaw=v->yaw;
}
void CityDrive(float dt){
    Vehicle *v=&VEH[P.vehIdx]; if(!v->active||v->wreck){ P.inVeh=false; return; }
    int t=v->type; static float steerSm=0; steerSm=Lerpf(steerSm,IN.move_x,fminf(1,dt*7.0f)); float thr=IN.move_y, st=steerSm, maxS=VMAX[t];
    if(thr>0.05f){ if(v->speed<-0.5f) v->speed+=VACC[t]*2.2f*dt; else v->speed+=VACC[t]*thr*dt*(1.0f-fmaxf(0,v->speed)/maxS*0.85f); }
    else if(thr<-0.05f){ if(v->speed>0.5f) v->speed-=13.0f*dt; else v->speed+=VACC[t]*thr*dt*0.7f; }
    else v->speed*=1.0f-fminf(1,0.45f*dt);
    if(IN.brake) v->speed*=1.0f-fminf(1,3.0f*dt);
    v->speed=Clampf(v->speed,-8.0f,maxS); v->braking=(thr<-0.05f&&v->speed>0.5f)||IN.brake;
    float auth=Clampf(fabsf(v->speed)/5.0f,0,1)*(1.0f-0.45f*fabsf(v->speed)/maxS), rate=(IN.brake&&fabsf(v->speed)>6)?3.2f:1.9f;
    v->yaw-=st*rate*auth*dt*(v->speed>=0?1.0f:-1.0f); v->life+=v->speed*dt*2.0f;
    Vector3 f=YawDir(v->yaw), c=V3Add(v->pos,V3Mul(f,v->speed*dt));
    float off=VHL[t]-VHW[t]-0.2f, rad=VHW[t]+0.45f;
    Vector3 pf=V3Add(c,V3Mul(f,off)), pr=V3Sub(c,V3Mul(f,off)), pf0=pf, pr0=pr;
    MoveCollide(&pf,rad,NULL); MoveCollide(&pr,rad,NULL);
    Vector3 corr=V3Mul(V3Add(V3Sub(pf,pf0),V3Sub(pr,pr0)),0.5f); v->pos=V3Add(c,corr); v->pos.y=0;
    v->turnCd-=dt; float push=V3Len(corr);
    if(push>0.03f&&fabsf(v->speed)>2.5f){ if(v->turnCd<=0){ v->turnCd=0.4f; v->hp-=fabsf(v->speed)*1.0f; GameHurtPlayer(fabsf(v->speed)*1.6f,V3Add(v->pos,f)); G.shake=fmaxf(G.shake,0.5f); Sfx(SFX_HURT,0.9f,0.7f); } v->speed*=1.0f-fminf(1,6.0f*dt); }
    for(int j=0;j<MAX_VEH;j++){ if(j==P.vehIdx||!VEH[j].active) continue; Vehicle *o=&VEH[j]; Vector3 d=V3Sub(o->pos,v->pos);
        float fd=V3Dot(d,f), ld=V3Dot(d,YawRight(v->yaw));
        if(fabsf(fd)<(VHL[t]+VHL[o->type])*0.92f&&fabsf(ld)<(VHW[t]+VHW[o->type])*1.05f){
            float closing=v->speed-o->speed*((V3Dot(YawDir(o->yaw),f)>0)?1.0f:-1.0f);
            if(fabsf(closing)>2.0f&&v->turnCd<=0){ v->turnCd=0.4f; v->hp-=fabsf(closing)*0.8f; o->hp-=fabsf(closing)*1.4f; GameHurtPlayer(fabsf(closing)*1.2f,o->pos); G.shake=fmaxf(G.shake,0.6f); Sfx(SFX_HURT,1.0f,0.6f); if(o->hp<=0) VehicleDestroy(j,true); }
            Vector3 push2=V3Mul(V3Norm(V3(d.x,0,d.z)),0.6f); o->pos=V3Add(o->pos,V3Mul(push2,1.0f)); v->pos=V3Sub(v->pos,V3Mul(push2,0.4f)); v->speed*=0.9f; o->speed=fmaxf(o->speed,v->speed*0.6f);
            if(!o->parked&&!o->wreck) o->parked=false; } }
    if(fabsf(v->speed)>4.0f){
        for(int i=0;i<MAX_CIT;i++){ Citizen *ci=&CIT[i]; if(!ci->active||ci->dead) continue; Vector3 d=V3Sub(ci->pos,v->pos); float fd=V3Dot(d,f), ld=V3Dot(d,YawRight(v->yaw));
            if(fabsf(ld)<VHW[t]+0.5f&&fd>-VHL[t]&&fd<VHL[t]+0.6f){ CityDamage(1,i,fabsf(v->speed)*9.0f,f,true); v->speed*=0.97f; } }
        for(int i=0;i<MAX_ENEMY;i++){ if(!EN[i].active||EN[i].dead) continue; Vector3 d=V3Sub(EN[i].pos,v->pos); float fd=V3Dot(d,f), ld=V3Dot(d,YawRight(v->yaw));
            if(fabsf(ld)<VHW[t]+0.6f&&fd>-VHL[t]&&fd<VHL[t]+0.7f&&fabsf(EN[i].pos.y-v->pos.y)<2.0f){ GameHurtEnemy(i,fabsf(v->speed)*10.0f,false,f,true,false); v->speed*=0.9f; } }
    }
    if(v->hp<v->maxHp*0.3f&&Rnd01()<0.25f) GameSmoke(V3(v->pos.x,1.2f,v->pos.z));
    if(v->hp<=0){ VehicleDestroy(P.vehIdx,true); return; }
    if(!P.inVeh) return;     /* died / was thrown out in the crash */
    P.pos=v->pos; P.yaw=v->yaw; P.speedN=0; P.vy=0; P.onGround=true;
}

/* ---------- damage / hit scan ---------- */
void CityDamage(int kind,int idx,float dmg,Vector3 dir,bool byPlayer){
    (void)dir;
    if(kind==1){ Citizen *c=&CIT[idx]; if(!c->active||c->dead) return; c->hp-=dmg;
        if(c->hp<=0){ c->dead=true; c->fall=0; c->deadT=0; ReleaseStall(c);
            if(byPlayer){ P.civKills++; P.score-=300; GameFeed("CIVILIAN CASUALTY  -300",(Color){255,90,90,255}); CityCrime(1,"CRIME WITNESSED - POLICE ALERTED"); }
            CityAlarm(c->pos,60);
        } else { c->flee=8; c->threat=P.pos; }
    } else if(kind==2){ Vehicle *v=&VEH[idx]; if(!v->active||v->wreck) return; v->hp-=dmg; if(v->hp<=0) VehicleDestroy(idx,byPlayer); }
}
bool CityHitScan(Vector3 o,Vector3 d,float maxT,float *tOut,int *kind,int *idx){
    float best=maxT; *kind=0; *idx=-1;
    for(int i=0;i<MAX_CIT;i++){ const Citizen *c=&CIT[i]; if(!c->active||c->dead) continue; float t;
        if(RayAABB(o,d,V3(c->pos.x-0.3f,c->pos.y,c->pos.z-0.3f),V3(c->pos.x+0.3f,c->pos.y+1.8f,c->pos.z+0.3f),&t)&&t<best){ best=t; *kind=1; *idx=i; } }
    for(int i=0;i<MAX_VEH;i++){ const Vehicle *v=&VEH[i]; if(!v->active||(v->player&&P.inVeh)) continue; float t;
        float s=fabsf(sinf(v->yaw)),cs=fabsf(cosf(v->yaw)), ex=s*VHL[v->type]+cs*VHW[v->type], ez=cs*VHL[v->type]+s*VHW[v->type], h=(v->type==3)?3.3f:(v->type==2)?1.5f:1.9f;
        if(RayAABB(o,d,V3(v->pos.x-ex,0,v->pos.z-ez),V3(v->pos.x+ex,h,v->pos.z+ez),&t)&&t<best){ best=t; *kind=2; *idx=i; } }
    *tOut=best; return *kind!=0;
}
void CityExplosion(Vector3 p,float radius,float dmg,bool byPlayer){
    for(int i=0;i<MAX_CIT;i++){ Citizen *c=&CIT[i]; if(!c->active||c->dead) continue; float d=V3DistXZ(p,c->pos);
        if(d<radius) CityDamage(1,i,dmg*(1.0f-d/radius)*1.2f,V3Sub(c->pos,p),byPlayer); }
    for(int i=0;i<MAX_VEH;i++){ Vehicle *v=&VEH[i]; if(!v->active||v->wreck) continue; float d=V3DistXZ(p,v->pos);
        if(d<radius+3) CityDamage(2,i,dmg*Clampf(1.0f-d/(radius+3),0,1)*0.9f,V3(0,0,0),byPlayer); }
    CityAlarm(p,100);
}

/* ---------- on-foot collision with vehicles (solid + run-over damage) ---------- */
static void PlayerVsVehicles(float dt){
    runHitCd-=dt;
    for(int i=0;i<MAX_VEH;i++){ Vehicle *v=&VEH[i]; if(!v->active||v->player||P.pos.y>2.0f) continue;
        Vector3 f=YawDir(v->yaw), r=YawRight(v->yaw), d=V3Sub(P.pos,v->pos); float fd=V3Dot(d,f), ld=V3Dot(d,r);
        float hl=VHL[v->type]+0.42f, hw=VHW[v->type]+0.42f;
        if(fabsf(fd)>=hl||fabsf(ld)>=hw) continue;
        float pf=hl-fabsf(fd), pl=hw-fabsf(ld);
        Vector3 out=(pf<pl)?V3Mul(f,fd>=0?pf:-pf):V3Mul(r,ld>=0?pl:-pl);
        P.pos=V3Add(P.pos,out);
        if(!v->parked&&!v->wreck&&fabsf(v->speed)>2.5f&&runHitCd<=0){
            runHitCd=0.8f; GameHurtPlayer(fabsf(v->speed)*5.0f,v->pos);
            P.pos=V3Add(P.pos,V3Mul(f,v->speed>0?1.5f:-1.5f)); v->speed*=0.85f; G.shake=fmaxf(G.shake,0.7f);
            GameFeed("HIT BY A VEHICLE",(Color){255,90,90,255});
        }
        MoveCollide(&P.pos,0.42f,NULL);
    }
}

/* ---------- crime & police ---------- */
void CityCrime(int stars,const char *why){
    int old=gWanted; gWanted=(int)Clampf((float)(gWanted+stars),0,5); gHeat=fmaxf(gHeat,30.0f+10.0f*gWanted);
    if(old==0){ dispatchT=2.5f; Sfx(SFX_RADIO,0.9f,1.0f); GameBanner("WANTED - POLICE DISPATCHED",(Color){90,140,255,255},3.5f); }
    GameFeed(why,(Color){255,90,90,255});
}
static bool SpawnPolice(void){
    int slot=-1; for(int i=0;i<MAX_VEH;i++) if(!VEH[i].active){ slot=i; break; } if(slot<0) return false;
    Vector3 pp=P.pos;
    for(int a=0;a<20;a++){
        int axis=GetRandomValue(0,1), dir=GetRandomValue(0,1)?1:-1; float perp=((axis==0)?pp.z:pp.x)+RndR(-150,150);
        int k=(int)roundf(perp/BLOCK_SZ); if(k<1)k=1; if(k>BLOCKS-1)k=BLOCKS-1;
        float t=Clampf(((axis==0)?pp.x:pp.z)+RndR(-150,150),40,WORLD_SZ-40);
        float lane=(axis==1)?k*BLOCK_SZ+5.0f*dir:k*BLOCK_SZ-5.0f*dir;
        Vector3 pos=(axis==0)?V3(t,0,lane):V3(lane,0,t); float d=V3DistXZ(pos,pp);
        if(d<80||d>160) continue; if(IsWaterAt(pos.x,pos.z)||!WorldFree(pos.x,pos.z,1.6f)) continue;
        bool clash=false; for(int j=0;j<MAX_VEH;j++) if(VEH[j].active&&V3DistXZ(VEH[j].pos,pos)<12) clash=true; if(clash) continue;
        Vehicle *v=&VEH[slot]; memset(v,0,sizeof(*v)); v->active=true; v->police=true; v->type=1; v->axis=axis; v->dir=dir; v->lineIdx=k; v->pos=pos;
        v->yaw=atan2f(pp.x-pos.x,pp.z-pos.z); v->speed=8.0f; v->hp=v->maxHp=VHP[1]*1.3f; v->col=CC(238,238,244); return true;
    }
    return false;
}
static void PoliceUpdate(Vehicle *v,int idx,float dt){
    v->pt+=dt; v->turnCd-=dt;
    float dist=V3DistXZ(v->pos,P.pos); float pspeed=(P.inVeh&&VEH[P.vehIdx].active)?fabsf(VEH[P.vehIdx].speed):0;
    float tspeed=VMAX[1]*0.75f; Vector3 f=YawDir(v->yaw);
    if(gWanted<=0){ tspeed=0; if(dist>90){ v->active=false; return; } }
    else {
        Vector3 want=V3Norm(V3(P.pos.x-v->pos.x,0,P.pos.z-v->pos.z)); Vector3 eye=V3(v->pos.x,1.0f,v->pos.z), tp=V3(P.pos.x,1.0f,P.pos.z);
        if(dist>4&&WorldRayT(eye,V3Norm(V3Sub(tp,eye)),dist,NULL)<dist-1.0f){ Vector3 fd; if(NavDir(v->pos,&fd)) want=fd; }
        float desired=atan2f(want.x,want.z), diff=AngleDiff(v->yaw,desired);
        if(v->stuck<0) { diff=-diff; tspeed=-6.0f; v->stuck+=dt; }
        else { if(fabsf(diff)>0.8f) tspeed*=0.5f; if(fabsf(v->speed)<1.2f&&dist>14) v->stuck+=dt; else if(v->stuck>0) v->stuck=fmaxf(0,v->stuck-dt); if(v->stuck>1.2f) v->stuck=-0.9f; }
        v->yaw+=Clampf(diff*3.0f,-2.2f,2.2f)*Clampf(fabsf(v->speed)/5.0f,0.3f,1.0f)*dt*(v->speed>=0?1.0f:-1.0f);
        if(!v->deployed&&dist<24&&pspeed<6.0f){            /* pull up and get out */
            tspeed=0; if(fabsf(v->speed)<2.5f){
                v->deployed=true; v->parked=true; int n=(gWanted>=3)?3:2; Vector3 r=YawRight(v->yaw);
                for(int k=0;k<n;k++){ Vector3 p=V3Add(v->pos,V3Mul(r,(k%2?-1.0f:1.0f)*(VHW[1]+1.4f))); p=V3Add(p,V3Mul(f,(k/2)*1.5f)); p.y=0; if(!WorldFree(p.x,p.z,0.4f)) p=v->pos; GameSpawnCop(p); }
                GameFeed("POLICE ON FOOT - FIGHT OR RUN!",(Color){90,140,255,255}); Sfx3D(SFX_RADIO,v->pos,1.0f); Sfx3D(SFX_ALERT,v->pos,1.0f); return; }
        }
    }
    float a=(tspeed>v->speed)?VACC[1]*1.4f:14.0f; v->speed+=Clampf(tspeed-v->speed,-a*dt,a*dt); v->braking=tspeed<v->speed-0.3f;
    Vector3 c=V3Add(v->pos,V3Mul(f,v->speed*dt)); float off=VHL[1]-VHW[1]-0.2f, rad=VHW[1]+0.45f;
    Vector3 pf=V3Add(c,V3Mul(f,off)), pr=V3Sub(c,V3Mul(f,off)), pf0=pf, pr0=pr; MoveCollide(&pf,rad,NULL); MoveCollide(&pr,rad,NULL);
    Vector3 corr=V3Mul(V3Add(V3Sub(pf,pf0),V3Sub(pr,pr0)),0.5f); v->pos=V3Add(c,corr); v->pos.y=0;
    if(V3Len(corr)>0.03f&&fabsf(v->speed)>2.5f) v->speed*=1.0f-fminf(1,5.0f*dt);
    for(int j=0;j<MAX_VEH;j++){ if(j==idx||!VEH[j].active||VEH[j].player) continue; Vehicle *o=&VEH[j]; Vector3 d=V3Sub(o->pos,v->pos);
        float fd=V3Dot(d,f), ld=V3Dot(d,YawRight(v->yaw));
        if(fabsf(fd)<(VHL[1]+VHL[o->type])*0.92f&&fabsf(ld)<(VHW[1]+VHW[o->type])*1.05f){
            if(v->turnCd<=0&&fabsf(v->speed)>3.0f){ v->turnCd=0.5f; o->hp-=fabsf(v->speed)*1.2f; v->hp-=fabsf(v->speed)*0.5f; if(o->hp<=0) VehicleDestroy(j,false); }
            o->pos=V3Add(o->pos,V3Mul(V3Norm(V3(d.x,0,d.z)),0.6f)); v->speed*=0.9f; } }
    if(P.inVeh&&VEH[P.vehIdx].active&&!VEH[P.vehIdx].wreck){ Vehicle *pv=&VEH[P.vehIdx]; Vector3 d=V3Sub(pv->pos,v->pos);
        float fd=V3Dot(d,f), ld=V3Dot(d,YawRight(v->yaw));
        if(fabsf(fd)<(VHL[1]+VHL[pv->type])*0.92f&&fabsf(ld)<(VHW[1]+VHW[pv->type])*1.05f&&v->turnCd<=0){
            Vector3 rel=V3Sub(V3Mul(f,v->speed),V3Mul(YawDir(pv->yaw),pv->speed)); float dmg=V3Len(rel);
            if(dmg>2.0f){ v->turnCd=0.5f; pv->hp-=dmg*1.0f; v->hp-=dmg*0.6f; GameHurtPlayer(dmg*0.6f,v->pos); G.shake=fmaxf(G.shake,0.7f); Sfx(SFX_HURT,1.0f,0.6f);
                pv->pos=V3Add(pv->pos,V3Mul(V3Norm(V3(d.x,0,d.z)),0.5f)); if(v->hp<=0) VehicleDestroy(idx,true); } } }
}
int CityPoliceCars(Vector3 *out,int max){ int n=0; for(int i=0;i<MAX_VEH&&n<max;i++) if(VEH[i].active&&VEH[i].police&&!VEH[i].wreck) out[n++]=VEH[i].pos; return n; }
float CityPoliceNear(void){ float b=1e9f; for(int i=0;i<MAX_VEH;i++) if(VEH[i].active&&VEH[i].police&&!VEH[i].wreck){ float d=V3DistXZ(VEH[i].pos,P.pos); if(d<b) b=d; } return b; }
/* siren loudness: every live cruiser adds, fades with distance (audible from ~200m, so you hear them coming) */
static void PoliceSiren(float dt){
    float vol=0;
    if(gWanted>0&&P.alive) for(int i=0;i<MAX_VEH;i++) if(VEH[i].active&&VEH[i].police&&!VEH[i].wreck){
        float t=Clampf(1.0f-V3DistXZ(VEH[i].pos,P.pos)/200.0f,0,1); vol+=t*t*(VEH[i].deployed?0.6f:1.0f); }
    SirenUpdate(Clampf(vol,0,1),dt);
}
static void CrimeUpdate(float dt){
    PoliceSiren(dt);
    if(gWanted<=0){ gHeat=0; gPoliceSeen=false; return; }
    gPoliceSeen=CityPoliceNear()<35.0f;
    for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active&&!EN[i].dead&&EN[i].cop&&EN[i].canSee){ gHeat=fmaxf(gHeat,15.0f); gPoliceSeen=true; }
    gHeat-=dt;
    if(gHeat<=0){ gWanted--; if(gWanted>0){ gHeat=25.0f; GameFeed("WANTED LEVEL DROPPING",(Color){120,200,255,255}); } else { GameFeed("WANTED LEVEL CLEARED - YOU LOST THE POLICE",(Color){120,255,150,255}); } return; }
    dispatchT-=dt;
    if(dispatchT<=0){
        dispatchT=fmaxf(4.0f,11.0f-1.8f*gWanted); int cars=0; for(int i=0;i<MAX_VEH;i++) if(VEH[i].active&&VEH[i].police&&!VEH[i].wreck&&!VEH[i].deployed) cars++;
        if(cars<gWanted+1&&SpawnPolice()){ GameFeed("POLICE UNITS INBOUND",(Color){90,140,255,255}); Sfx(SFX_RADIO,0.7f,0.9f); }
    }
}

/* ---------- lifecycle ---------- */
void CityReset(void){
    memset(CIT,0,sizeof(CIT)); memset(VEH,0,sizeof(VEH)); cityTime=0; spawnT=0.25f; gWanted=0; gHeat=0; dispatchT=0; runHitCd=0; gPoliceSeen=false; SirenUpdate(0,0);
    for(int i=0;i<nStall;i++) STALLS[i].taken=false;
    pullerLook=MakeCitizen(); pullerLook.skirt=true; pullerLook.skirtCol=CC(70,100,160); pullerLook.hatType=3; pullerLook.hat=CC(200,200,190);
    int n=0; for(int a=0;a<400&&n<CitTarget();a++) if(SpawnCitizen(20,200)) n++;
    n=0; for(int a=0;a<200&&n<VehTarget();a++) if(SpawnTraffic(35,230)) n++;
}
void CityUpdate(float dt){
    cityTime+=dt; spawnT-=dt;
    if(spawnT<=0){
        spawnT=0.25f; int nc=0,nv=0; for(int i=0;i<MAX_CIT;i++) if(CIT[i].active) nc++; for(int i=0;i<MAX_VEH;i++) if(VEH[i].active) nv++;
        for(int k=0;k<3&&nc<CitTarget();k++) if(SpawnCitizen(80,220)) nc++;
        for(int k=0;k<2&&nv<VehTarget();k++) if(SpawnTraffic(100,250)) nv++;
    }
    for(int i=0;i<MAX_CIT;i++) if(CIT[i].active) CitizenUpdate(&CIT[i],dt);
    CrimeUpdate(dt); if(P.alive&&!P.inVeh) PlayerVsVehicles(dt);
    for(int i=0;i<MAX_VEH;i++){ Vehicle *v=&VEH[i]; if(!v->active) continue;
        if(!v->player&&V3DistXZ(v->pos,P.pos)>330){ v->active=false; continue; }
        if(v->player) continue;
        if(v->parked||v->wreck){ v->speed*=1.0f-fminf(1,2.0f*dt); v->pos=V3Add(v->pos,V3Mul(YawDir(v->yaw),v->speed*dt)); v->braking=false;
            if(v->wreck&&Rnd01()<0.05f) GameSmoke(V3(v->pos.x,1.0f,v->pos.z)); continue; }
        if(v->police){ PoliceUpdate(v,i,dt); continue; }
        TrafficUpdate(v,i,dt); }
}
void CityDraw(Vector3 cam,Vector3 fwd){
    float R=(SV.st.drawDist==0)?170.0f:(SV.st.drawDist==1)?240.0f:320.0f;
    for(int i=0;i<MAX_CIT;i++){ const Citizen *c=&CIT[i]; if(!c->active) continue;
        float dx=c->pos.x-cam.x,dz=c->pos.z-cam.z,d2=dx*dx+dz*dz; if(d2>R*R) continue; if(dx*fwd.x+dz*fwd.z<-15&&d2>400) continue;
        float d=sqrtf(d2);
        if(d<42) DrawHuman(&c->look,c->pos,c->yaw,c->walk,c->speedN,0,-1,false,c->dead?c->fall:0,0,0);
        else DrawHumanLow(&c->look,c->pos,c->yaw,c->walk,c->speedN,c->dead?c->fall:0);
        if(SV.st.blobShadow&&!c->dead&&d<70) DrawFlat(c->pos.x,0.14f,c->pos.z,0.9f,0.9f,(Color){0,0,0,60}); }
    for(int i=0;i<MAX_VEH;i++){ const Vehicle *v=&VEH[i]; if(!v->active) continue;
        float dx=v->pos.x-cam.x,dz=v->pos.z-cam.z,d2=dx*dx+dz*dz; if(d2>(R+60)*(R+60)) continue; if(dx*fwd.x+dz*fwd.z<-25&&d2>900) continue;
        DrawVehicleAt(v->type,V3(v->pos.x,0,v->pos.z),v->yaw,v->col,v->braking);
        if(v->police&&!v->wreck){
            int ph=((int)(cityTime*10.0f))&7; bool rOn=(ph==0||ph==2), bOn=(ph==4||ph==6);       /* GTA-style double flash */
            Color rC=rOn?CC(255,30,30):CC(80,8,8), bC=bOn?CC(40,90,255):CC(10,20,70), blk=CC(14,16,24);
            rlPushMatrix(); rlTranslatef(v->pos.x,0,v->pos.z); rlRotatef(v->yaw*RAD2DEG,0,1,0);
            for(int s=-1;s<=1;s+=2){ BoxC(V3(s*0.758f,0.78f,-0.1f),V3(0.02f,0.62f,1.25f),blk); BoxC(V3(s*0.762f,1.02f,-0.1f),V3(0.02f,0.09f,1.15f),CC(245,245,250)); }
            BoxC(V3(0,1.46f,0.8f),V3(1.2f,0.02f,0.8f),blk);                                      /* black hood */
            BoxC(V3(0,0.5f,1.36f),V3(1.2f,0.3f,0.1f),CC(16,16,20));                              /* push bar */
            BoxC(V3(0,1.72f,-0.1f),V3(1.2f,0.1f,0.4f),CC(18,18,24));                             /* light bar base */
            BoxC(V3(-0.32f,1.82f,-0.1f),V3(0.5f,0.16f,0.3f),rC); BoxC(V3(0.32f,1.82f,-0.1f),V3(0.5f,0.16f,0.3f),bC);
            BoxC(V3(-0.45f,0.95f,1.32f),V3(0.25f,0.1f,0.05f),rC); BoxC(V3(0.45f,0.95f,1.32f),V3(0.25f,0.1f,0.05f),bC);
            BoxC(V3(-0.45f,1.0f,-1.32f),V3(0.25f,0.1f,0.05f),rC); BoxC(V3(0.45f,1.0f,-1.32f),V3(0.25f,0.1f,0.05f),bC);
            rlPopMatrix();
            if(rOn) DrawFlat(v->pos.x,0.17f,v->pos.z,11,11,(Color){255,30,30,70});               /* light pool on the road */
            else if(bOn) DrawFlat(v->pos.x,0.17f,v->pos.z,11,11,(Color){40,90,255,70}); }
        if(v->type==0&&!v->wreck&&!v->player) DrawHumanLow(&pullerLook,V3(v->pos.x+sinf(v->yaw)*0.55f,0.42f,v->pos.z+cosf(v->yaw)*0.55f),v->yaw,v->life,fminf(1,fabsf(v->speed)/3),0);
        if(SV.st.blobShadow&&d2<14000) DrawFlat(v->pos.x,0.1f,v->pos.z,VHW[v->type]*2+0.8f,VHW[v->type]*2+0.8f,(Color){0,0,0,55}); }
}
