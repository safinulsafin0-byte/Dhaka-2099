/* In-game HUD: minimap, objective arrow, ammo/health, crosshair, touch controls, big map */
#include "ui_internal.h"

static bool PtInRect(Vector2 p,Rectangle r){ return CheckCollisionPointRec(p,r); }
static Vector2 Rot(Vector2 v,float a){ float c=cosf(a),s=sinf(a); return (Vector2){v.x*c-v.y*s,v.x*s+v.y*c}; }
static Vector2 W2S(Vector3 p,bool *ok){
    Vector3 d=V3Sub(p,CAM.position); Vector3 f=V3Norm(V3Sub(CAM.target,CAM.position));
    *ok=V3Dot(d,f)>0.5f; Vector2 s=GetWorldToScreen(p,CAM); return (Vector2){s.x/uiScale,s.y/uiScale};
}

/* ---------- touch controls ---------- */
enum { TB_FIRE,TB_AIM,TB_RELOAD,TB_JUMP,TB_GREN,TB_C4,TB_DET,TB_SWAP,TB_USE,TB_PAUSE,TB_N };
static const float TBO[TB_N][3]={{125,135,66},{255,70,40},{255,175,36},{70,270,34},{190,265,36},{325,120,34},{325,215,32},{120,350,30},{225,350,32},{0,0,24}};
static const char *TBL[TB_N]={"FIRE","AIM","RLD","JUMP","GREN","C4","BOOM","SWAP","USE","II"};
static int joyId=-1,lookId=-1; static Vector2 joyBase,joyKnob,lookLast;
static Vector2 TBPos(int i){ if(i==TB_PAUSE) return (Vector2){uiW-36,38}; return (Vector2){uiW-TBO[i][0],VIRT_H-TBO[i][1]}; }
static bool c4Active(void){ for(int i=0;i<MAX_C4;i++) if(C4S[i].active) return true; return false; }
static bool BtnVisible(int b){
    if(b==TB_PAUSE||b==TB_JUMP) return true;
    if(b==TB_USE) return CityNearVehicle();
    if(P.inVeh) return false;
    if(b==TB_DET) return c4Active();
    return true;
}

void HudTouchInput(void){
    bool held[TB_N]={0},pressed[TB_N]={0};
    for(int i=0;i<uiNPtr;i++){
        Ptr *p=&uiPtr[i];
        for(int b=0;b<TB_N;b++){
            if(!BtnVisible(b)) continue;
            Vector2 c=TBPos(b); float r=TBO[b][2]*1.15f;
            if(Vector2Distance(p->pos,c)<r){ held[b]=true; if(p->pressed) pressed[b]=true; }
        }
        if(p->pressed){
            bool onBtn=false; for(int b=0;b<TB_N;b++) if(BtnVisible(b)&&Vector2Distance(p->pos,TBPos(b))<TBO[b][2]*1.4f) onBtn=true;
            if(!onBtn){
                if(joyId<0&&p->pos.x<uiW*0.36f&&p->pos.y>VIRT_H*0.28f){ joyId=p->id; joyBase=p->pos; joyKnob=p->pos; }
                else if(lookId<0&&p->pos.x>=uiW*0.30f){ lookId=p->id; lookLast=p->pos; }
            }
        }
    }
    bool jf=false,lf=false; Vector2 jv={0,0};
    for(int i=0;i<uiNPtr;i++){
        Ptr *p=&uiPtr[i];
        if(p->id==joyId){ jf=true; joyKnob=p->pos; Vector2 d=Vector2Subtract(p->pos,joyBase); float l=Vector2Length(d); if(l>90){ d=Vector2Scale(d,90.0f/l); joyKnob=Vector2Add(joyBase,d); } jv=Vector2Scale(d,1.0f/90.0f); }
        if(p->id==lookId){ lf=true; float k=0.0040f*SV.st.sens; IN.look_x+=(p->pos.x-lookLast.x)*k; IN.look_y+=(p->pos.y-lookLast.y)*k; lookLast=p->pos; }
    }
    if(!jf) joyId=-1; if(!lf) lookId=-1;
    IN.move_x+=jv.x; IN.move_y+=-jv.y; if(Vector2Length(jv)>0.93f) IN.sprint=true;
    IN.fire|=held[TB_FIRE]; IN.firePressed|=pressed[TB_FIRE]; IN.aim|=held[TB_AIM]; IN.jump|=pressed[TB_JUMP];
    IN.reload|=pressed[TB_RELOAD]; IN.grenade|=pressed[TB_GREN]; IN.c4|=pressed[TB_C4]; IN.det|=pressed[TB_DET]; IN.swap|=pressed[TB_SWAP]; IN.pause|=pressed[TB_PAUSE]; IN.use|=pressed[TB_USE]; if(P.inVeh) IN.brake|=held[TB_JUMP];
}
static void DrawTouch(void){
    Vector2 jb=(joyId>=0)?joyBase:(Vector2){150,VIRT_H-150}, jk=(joyId>=0)?joyKnob:jb;
    DrawCircleV(jb,90,(Color){255,255,255,28}); DrawCircleLines((int)jb.x,(int)jb.y,90,(Color){255,255,255,90});
    DrawCircleV(jk,38,(Color){255,255,255,joyId>=0?110:60});
    for(int b=0;b<TB_N;b++){
        if(!BtnVisible(b)) continue;
        Vector2 c=TBPos(b); float r=TBO[b][2]; bool on=false;
        for(int i=0;i<uiNPtr;i++) if(Vector2Distance(uiPtr[i].pos,c)<r*1.15f) on=true;
        Color col=(b==TB_FIRE)?COL_RED:(b==TB_DET)?COL_GOLD:(b==TB_GREN||b==TB_C4)?(Color){255,150,60,255}:(Color){255,255,255,255};
        DrawCircleV(c,r,(Color){col.r,col.g,col.b,(unsigned char)(on?120:50)}); DrawCircleLines((int)c.x,(int)c.y,r,(Color){col.r,col.g,col.b,170});
        int sz=(b==TB_FIRE)?20:13; TextC((b==TB_JUMP&&P.inVeh)?"BRAKE":TBL[b],c.x,c.y-sz/2.0f,sz,COL_TEXT);
        if(b==TB_GREN) TextC(TextFormat("x%d",P.gren),c.x,c.y+r+2,13,COL_TEXT);
        if(b==TB_C4) TextC(TextFormat("x%d",P.bombs),c.x,c.y+r+2,13,COL_TEXT);
    }
}

/* ---------- map helpers ---------- */
static float MapS(void){ return MAP_PX/(WORLD_SZ+MAP_MARGIN*2); }
bool HudMapOpen(void){ return G.mapBig; }

static void BigMap(void){
    DrawRectangle(0,0,(int)uiW,(int)VIRT_H,(Color){0,0,0,185});
    float side=VIRT_H-80; Rectangle r={uiW/2-side/2,40,side,side};
    float tex=(float)MAP_PX;
    DrawTexturePro(mapTex,(Rectangle){0,0,tex,tex},(Rectangle){r.x-side*MAP_MARGIN/WORLD_SZ,r.y-side*MAP_MARGIN/WORLD_SZ,side*(WORLD_SZ+2*MAP_MARGIN)/WORLD_SZ,side*(WORLD_SZ+2*MAP_MARGIN)/WORLD_SZ},(Vector2){0,0},0,WHITE);
    DrawRectangleLinesEx(r,2,COL_GREEN);
    float k=side/WORLD_SZ;
#define MP(v) (Vector2){r.x+(v).x*k,r.y+(v).z*k}
    for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active&&!EN[i].dead){ Vector2 p=MP(EN[i].pos); DrawCircleV(p,3,COL_RED); }
    Vector2 b=MP(BASE.center); DrawCircleLines((int)b.x,(int)b.y,12,COL_RED); TextC("ENEMY BASE",b.x,b.y-26,14,COL_RED);
    Vector2 pp=MP(P.pos); float a=G.camYaw-PI; (void)a;
    Vector2 d=(Vector2){-sinf(G.camYaw)*0+sinf(G.camYaw),cosf(G.camYaw)};
    Vector2 tip=Vector2Add(pp,Vector2Scale(d,12)), l=Vector2Add(pp,Vector2Scale((Vector2){-d.y,d.x},6)), rr=Vector2Add(pp,Vector2Scale((Vector2){d.y,-d.x},6));
    Tri(tip,l,rr,COL_GOLD); DrawCircleV(pp,3,WHITE);
    Vector2 st=MP(M.start); DrawCircleV(st,5,COL_GREEN); TextC("START",st.x,st.y+8,12,COL_GREEN);
#undef MP
    TextC(TextFormat("YOU ARE IN: %s",DistrictAt(P.pos.x,P.pos.z)),uiW/2,8,22,COL_TEXT);
    TextC("PRESS M OR TAP TO CLOSE",uiW/2,VIRT_H-30,16,COL_DIM);
}

static void HudStar(float cx,float cy,float r,Color col){
    Vector2 p[10]; for(int i=0;i<10;i++){ float a=-PI/2+i*PI/5, rr=(i%2==0)?r:r*0.42f; p[i]=(Vector2){cx+cosf(a)*rr,cy+sinf(a)*rr}; }
    for(int i=0;i<10;i++) Tri((Vector2){cx,cy},p[i],p[(i+1)%10],col);
}
static void MiniMap(void){
    Rectangle mm={14,14,188,188}; Vector2 c={mm.x+mm.width/2,mm.y+mm.height/2};
    float viewM=150.0f, ppm=mm.width/viewM, s=MapS();
    float D=mm.width*1.5f; float S=D/ppm*s; float u=(P.pos.x+MAP_MARGIN)*s, v=(P.pos.z+MAP_MARGIN)*s;
    DrawRectangleRec(mm,(Color){8,12,18,255});
    BeginScissorMode((int)(mm.x*uiScale),(int)(mm.y*uiScale),(int)(mm.width*uiScale),(int)(mm.height*uiScale));
    float rotDeg=(G.camYaw-PI)*RAD2DEG, th=G.camYaw-PI;
    DrawTexturePro(mapTex,(Rectangle){u-S/2,v-S/2,S,S},(Rectangle){c.x,c.y,D,D},(Vector2){D/2,D/2},rotDeg,WHITE);
#define MPT(w) Vector2Add(c,Rot((Vector2){((w).x-P.pos.x)*ppm,((w).z-P.pos.z)*ppm},th))
    for(int i=0;i<MAX_PICKUP;i++) if(PK[i].active){ Vector2 p=MPT(PK[i].pos); static const Color pc[9]={{120,255,150,255},{255,220,100,255},{180,230,120,255},{255,150,80,255},{255,210,90,255},{110,170,255,255},{220,140,255,255},{255,220,80,255},{80,255,160,255}}; DrawRectangle((int)p.x-2,(int)p.y-2,4,4,pc[PK[i].type]); }
    for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active&&!EN[i].dead){ Vector2 p=MPT(EN[i].pos); if(V3DistXZ(EN[i].pos,P.pos)<viewM*0.62f){ Color ec=EN[i].state==1?COL_RED:(EN[i].spot>0.3f?COL_GOLD:(Color){255,120,90,255}); DrawCircleV(p,EN[i].state==1?4.0f:3.0f,ec);
        if(EN[i].state==0){ Vector2 fd=Rot((Vector2){sinf(EN[i].yaw)*9,cosf(EN[i].yaw)*9},th); DrawLineEx(p,Vector2Add(p,fd),1.5f,ec); } } }
    for(int i=0;i<MAX_C4;i++) if(C4S[i].active){ Vector2 p=MPT(C4S[i].pos); DrawCircleV(p,3,COL_GOLD); }
    { Vector3 pc[16]; int np=CityPoliceCars(pc,16); bool bl=((int)(G.playClock*5))&1;
      for(int i=0;i<np;i++){ Vector2 p=MPT(pc[i]); DrawRectangle((int)p.x-5,(int)p.y-5,10,10,WHITE); DrawRectangle((int)p.x-4,(int)p.y-4,8,8,bl?(Color){255,40,40,255}:(Color){50,100,255,255}); } }
    Vector3 op; char lab[24]; if(GameObjective(&op,lab,sizeof(lab))){
        Vector2 o=Rot((Vector2){(op.x-P.pos.x)*ppm,(op.z-P.pos.z)*ppm},th); float l=Vector2Length(o), lim=mm.width/2-9;
        if(l>lim) o=Vector2Scale(o,lim/l); Vector2 p=Vector2Add(c,o);
        Tri((Vector2){p.x,p.y-8},(Vector2){p.x-7,p.y},(Vector2){p.x+7,p.y},COL_GOLD); Tri((Vector2){p.x,p.y+8},(Vector2){p.x+7,p.y},(Vector2){p.x-7,p.y},COL_GOLD); }
    Vector2 nrt=Vector2Add(c,Rot((Vector2){0,-(mm.width/2-12)},th)); TextC("N",nrt.x,nrt.y-7,14,COL_RED);
#undef MPT
    Tri((Vector2){c.x,c.y-8},(Vector2){c.x-6,c.y+7},(Vector2){c.x+6,c.y+7},WHITE); Tri((Vector2){c.x,c.y-3},(Vector2){c.x-3,c.y+5},(Vector2){c.x+3,c.y+5},COL_GREEN);
    EndScissorMode();
    DrawRectangleLinesEx(mm,2,COL_GREEN);
    DrawRectangle((int)mm.x,(int)(mm.y+mm.height+4),(int)mm.width,22,(Color){8,12,18,220});
    TextC(DistrictAt(P.pos.x,P.pos.z),c.x,mm.y+mm.height+8,14,COL_GOLD);
    if(uiTapped&&PtInRect(uiTapPos,mm)) G.mapBig=true;
}

/* ---------- HUD ---------- */
static void Bar(float x,float y,float w,float h,float f,Color c){
    DrawRectangle((int)x,(int)y,(int)w,(int)h,(Color){0,0,0,160}); DrawRectangle((int)x+2,(int)y+2,(int)((w-4)*Clampf(f,0,1)),(int)h-4,c);
    DrawRectangleLinesEx((Rectangle){x,y,w,h},1,(Color){255,255,255,80});
}
void HudDraw(void){
    if(G.hurtT>0||P.hp<P.maxHp*0.3f){
        float a=(G.hurtT>0)?G.hurtT/0.6f*0.55f:0.18f+0.12f*sinf((float)GetTime()*6);
        DrawRectangleGradientH(0,0,(int)(uiW*0.2f),(int)VIRT_H,(Color){200,0,0,(unsigned char)(255*a)},(Color){200,0,0,0});
        DrawRectangleGradientH((int)(uiW*0.8f),0,(int)(uiW*0.2f)+1,(int)VIRT_H,(Color){200,0,0,0},(Color){200,0,0,(unsigned char)(255*a)});
    }
    if(G.flashWhite>0.01f) DrawRectangle(0,0,(int)uiW,(int)VIRT_H,(Color){255,255,255,(unsigned char)(255*G.flashWhite)});
    Vector2 cc={uiW/2,VIRT_H/2};
    /* crosshair */
    const WeaponDef *w=&WEAPONS[P.wid[P.cw]];
    if(!P.inVeh){
    float sp=8+P.recoil*2.5f+(P.speedN>0.3f?5:0)+(P.aiming?-3:0); sp+=w->spread*1.5f;
    Color xc=(G.hitT>0)?(G.hitHead?COL_GOLD:COL_RED):(Color){255,255,255,230};
    if(P.aiming&&w->sfx==SFX_SNIPER){ DrawCircleLines((int)cc.x,(int)cc.y,170,(Color){0,0,0,200}); DrawLine((int)cc.x-170,(int)cc.y,(int)cc.x+170,(int)cc.y,(Color){0,0,0,200}); DrawLine((int)cc.x,(int)cc.y-170,(int)cc.x,(int)cc.y+170,(Color){0,0,0,200}); }
    DrawLineEx((Vector2){cc.x-sp-9,cc.y},(Vector2){cc.x-sp,cc.y},2,xc); DrawLineEx((Vector2){cc.x+sp,cc.y},(Vector2){cc.x+sp+9,cc.y},2,xc);
    DrawLineEx((Vector2){cc.x,cc.y-sp-9},(Vector2){cc.x,cc.y-sp},2,xc); DrawLineEx((Vector2){cc.x,cc.y+sp},(Vector2){cc.x,cc.y+sp+9},2,xc);
    DrawCircleV(cc,1.6f,xc);
    if(G.hitT>0){ float h=G.hitHead?12:9; Color hc=G.hitHead?COL_GOLD:COL_RED; DrawLineEx((Vector2){cc.x-h,cc.y-h},(Vector2){cc.x-4,cc.y-4},3,hc); DrawLineEx((Vector2){cc.x+h,cc.y-h},(Vector2){cc.x+4,cc.y-4},3,hc); DrawLineEx((Vector2){cc.x-h,cc.y+h},(Vector2){cc.x-4,cc.y+4},3,hc); DrawLineEx((Vector2){cc.x+h,cc.y+h},(Vector2){cc.x+4,cc.y+4},3,hc); }
    if(P.reloading){ float f=1.0f-P.reloadT/w->reload; Bar(cc.x-50,cc.y+48,100,10,f,COL_GOLD); TextC("RELOADING",cc.x,cc.y+62,14,COL_TEXT); }
    }
    /* damage direction */
    if(G.hurtT>0){ Vector2 d=Rot((Vector2){0,-120},-G.hurtAng); Vector2 p=Vector2Add(cc,d); Vector2 n=Vector2Normalize(d);
        Tri(Vector2Add(p,Vector2Scale(n,18)),Vector2Add(p,Vector2Scale((Vector2){-n.y,n.x},12)),Vector2Add(p,Vector2Scale((Vector2){n.y,-n.x},12)),(Color){255,40,40,(unsigned char)(255*Clampf(G.hurtT/0.6f,0,1))}); }
    /* enemy health bars + objective marker */
    for(int i=0;i<MAX_ENEMY;i++){ const Enemy *e=&EN[i]; if(!e->active||e->dead||e->state!=1) continue; if(V3Dist(e->pos,P.pos)>70) continue;
        bool ok; Vector2 s=W2S(V3(e->pos.x,e->pos.y+2.15f,e->pos.z),&ok); if(!ok) continue;
        Bar(s.x-20,s.y,40,6,e->hp/e->maxHp,(e->type==EN_COMMANDER)?COL_GOLD:COL_RED); }
    Vector3 op; char lab[24]; bool hasObj=GameObjective(&op,lab,sizeof(lab));
    if(hasObj){
        float dist=V3DistXZ(op,P.pos); float rel=AngleDiff(G.camYaw,atan2f(op.x-P.pos.x,op.z-P.pos.z));
        Vector2 ac={uiW/2,74}; float ang=-rel; float R=24;
        DrawCircleV(ac,34,(Color){0,0,0,130}); DrawCircleLines((int)ac.x,(int)ac.y,34,COL_GOLD);
        Vector2 tip=Vector2Add(ac,Rot((Vector2){0,-R},ang)), l=Vector2Add(ac,Rot((Vector2){-R*0.7f,R*0.75f},ang)), r=Vector2Add(ac,Rot((Vector2){R*0.7f,R*0.75f},ang)), nt=Vector2Add(ac,Rot((Vector2){0,R*0.3f},ang));
        Tri(tip,l,nt,COL_GOLD); Tri(tip,nt,r,(Color){255,160,40,255});
        TextC(TextFormat("%s  %dm",lab,(int)dist),uiW/2,112,16,COL_TEXT);
        bool ok; Vector2 s=W2S(V3(op.x,op.y+(strcmp(lab,"HOSTILE")==0?1.4f:5.0f),op.z),&ok);
        if(ok&&s.x>20&&s.x<uiW-20&&s.y>20&&s.y<VIRT_H-20){ Tri((Vector2){s.x-9,s.y-14},(Vector2){s.x+9,s.y-14},(Vector2){s.x,s.y},COL_GOLD); TextC(TextFormat("%dm",(int)dist),s.x,s.y-32,14,COL_GOLD); }
    }
    MiniMap();
    /* ---- wanted level (GTA style) ---- */
    if(gWanted>0){
        bool evade=!gPoliceSeen, blink=evade&&(((int)(G.playClock*3))&1); bool bl=((int)(G.playClock*5))&1;
        for(int i=0;i<5;i++){ float sx=14+14+i*29, sy=248; Color c;
            if(i<gWanted) c=blink?(Color){255,255,255,60}:(Color){255,206,40,255}; else c=(Color){255,255,255,50};
            HudStar(sx+1.5f,sy+2,13,(Color){0,0,0,150}); HudStar(sx,sy,13,c); }
        Color tc=evade?(Color){180,200,255,255}:(bl?COL_RED:(Color){90,140,255,255});
        TextC(evade?"WANTED - EVADING":"WANTED - POLICE IN PURSUIT",14+72,270,12,tc);
        Bar(14,286,146,6,Clampf(gHeat/70.0f,0,1),(Color){90,140,255,255});
        /* police light flash on screen edges when a cruiser is close */
        float pd=CityPoliceNear();
        if(pd<70){ float k=Clampf(1.0f-pd/70.0f,0,1)*0.6f; int ph=((int)(G.playClock*10.0f))&7; bool rOn=(ph==0||ph==2), bOn=(ph==4||ph==6);
            unsigned char ra=(unsigned char)(255*k*(rOn?1.0f:0.12f)), ba=(unsigned char)(255*k*(bOn?1.0f:0.12f));
            DrawRectangleGradientH(0,0,(int)(uiW*0.22f),(int)VIRT_H,(Color){255,30,30,ra},(Color){255,30,30,0});
            DrawRectangleGradientH((int)(uiW*0.78f),0,(int)(uiW*0.22f)+1,(int)VIRT_H,(Color){40,90,255,0},(Color){40,90,255,ba}); }
    }
    /* ---- detection system ---- */
    if(!M.free){
        const char *dn[3]={"HIDDEN","SUSPICIOUS","DETECTED"}; Color dc[3]={COL_GREEN,COL_GOLD,COL_RED};
        float bw=170; DrawRectangle((int)(uiW/2-bw/2),8,(int)bw,8,(Color){0,0,0,160});
        DrawRectangle((int)(uiW/2-bw/2)+1,9,(int)((bw-2)*Clampf(G.detect,0,1)),6,dc[G.detState]); DrawRectangleLinesEx((Rectangle){uiW/2-bw/2,8,bw,8},1,(Color){255,255,255,90});
        TextC(dn[G.detState],uiW/2,17,11,dc[G.detState]);
        for(int i=0;i<MAX_ENEMY;i++){ const Enemy *e=&EN[i]; if(!e->active||e->dead) continue;
            bool aware=(e->state==0&&e->spot>0.04f), alarmed=(e->state==1&&e->alertT>0); if(!aware&&!alarmed) continue;
            float dd=V3Dist(e->pos,P.pos); if(dd>110) continue;
            float lv=alarmed?1.0f:Clampf(e->spot,0,1); Color ic=alarmed?COL_RED:ColMix(COL_GOLD,COL_RED,lv);
            bool ok; Vector2 s=W2S(V3(e->pos.x,e->pos.y+2.5f,e->pos.z),&ok);
            if(ok&&s.x>20&&s.x<uiW-20&&s.y>20&&s.y<VIRT_H-20){
                DrawCircleV(s,13,(Color){0,0,0,170}); DrawCircleSector(s,13,0,360*lv,24,(Color){ic.r,ic.g,ic.b,200}); DrawCircleV(s,9,(Color){10,14,18,230});
                TextC(alarmed?"!":"?",s.x,s.y-8,16,ic);
            } else if(e->spot>0.25f||alarmed){
                float rel=AngleDiff(G.camYaw,atan2f(e->pos.x-P.pos.x,e->pos.z-P.pos.z)); Vector2 dir=Rot((Vector2){0,-1},-rel);
                Vector2 p=Vector2Add(cc,Vector2Scale(dir,185)); Vector2 n=dir, sd=(Vector2){-n.y,n.x};
                Tri(Vector2Add(p,Vector2Scale(n,14)),Vector2Add(p,Vector2Scale(sd,10)),Vector2Add(p,Vector2Scale(sd,-10)),(Color){ic.r,ic.g,ic.b,(unsigned char)(130+120*lv)});
            }
        }
    }
    /* armor + adrenaline */
    if(P.armor>0){ Bar(uiW/2-150,VIRT_H-62,300,10,P.armor/100.0f,(Color){90,150,255,255}); TextC("ARMOR",uiW/2+120,VIRT_H-63,10,COL_TEXT); }
    if(P.adren>0) TextC(TextFormat("ADRENALINE %ds",(int)P.adren+1),uiW/2,VIRT_H-80,13,(Color){220,140,255,255});
    /* objective panel */
    Rectangle op2={14,238,230,96}; Panel(op2,(Color){8,14,20,200});
    DrawText(M.free?"FREE ROAM":TextFormat("MISSION %d  %s",M.idx+1,M.name),(int)op2.x+10,(int)op2.y+8,14,COL_GOLD);
    if(M.free){ DrawText("EXPLORE DHAKA",(int)op2.x+10,(int)op2.y+30,14,COL_TEXT); DrawText("Drive, shop-watch, survive.",(int)op2.x+10,(int)op2.y+52,13,COL_DIM); DrawText(TextFormat("TIME %02d:%02d",(int)M.time/60,(int)M.time%60),(int)op2.x+10,(int)op2.y+72,13,COL_DIM); }
    else {
    const char *obj=(M.stage==0)?"REACH THE ENEMY BASE":(M.stage==1)?"ELIMINATE ALL HOSTILES":"DESTROY THE WEAPONS CACHE";
    if(M.stage==3) obj="MISSION COMPLETE";
    DrawText(obj,(int)op2.x+10,(int)op2.y+30,14,COL_TEXT);
    DrawText(TextFormat("ROUND %d/%d    HOSTILES LEFT %d",M.round,M.rounds,M.total-M.killed),(int)op2.x+10,(int)op2.y+52,13,COL_RED);
    int sec=(int)M.time; DrawText(TextFormat("TIME %02d:%02d    KILLS %d",sec/60,sec%60,P.kills),(int)op2.x+10,(int)op2.y+72,13,COL_DIM);
    }
    /* vehicle panel */
    if(P.inVeh){
        const Vehicle *v=&VEH[P.vehIdx]; static const char *vn[4]={"RICKSHAW","CNG AUTO","SEDAN","BUS"};
        float wx=uiW-(UiTouchMode()?352:300); Panel((Rectangle){wx,12,UiTouchMode()?268:288,100},(Color){8,14,20,200});
        DrawText(vn[v->type],(int)wx+12,22,16,COL_TEXT); DrawText(TextFormat("%d",(int)fabsf(v->speed*3.6f)),(int)wx+12,44,44,COL_TEXT); DrawText("km/h",(int)wx+(v->speed*3.6f>=100?118:96),(int)76,16,COL_DIM);
        Bar(wx+150,30,120,12,v->hp/v->maxHp,v->hp<v->maxHp*0.3f?COL_RED:COL_GREEN); DrawText("VEHICLE",(int)wx+150,48,11,COL_DIM);
    }
    /* weapon panel */
    if(!P.inVeh){
    float wx=uiW-(UiTouchMode()?352:300), wy=12; Panel((Rectangle){wx,wy,UiTouchMode()?268:288,112},(Color){8,14,20,200});
    DrawText(w->name,(int)wx+12,(int)wy+8,16,COL_TEXT); DrawText(w->type,(int)wx+12,(int)wy+27,11,COL_DIM);
    DrawText(TextFormat("%02d",P.mag[P.cw]),(int)wx+12,(int)wy+44,40,P.mag[P.cw]<=w->mag/4?COL_RED:COL_TEXT);
    DrawText(TextFormat("/ %d",P.res[P.cw]),(int)wx+78,(int)wy+60,20,COL_DIM);
    const WeaponDef *o=&WEAPONS[P.wid[P.cw^1]]; DrawText(TextFormat("[Q] %s  %d/%d",o->name,P.mag[P.cw^1],P.res[P.cw^1]),(int)wx+12,(int)wy+92,11,COL_DIM);
    DrawText(TextFormat("GRENADES %d",P.gren),(int)wx+160,(int)wy+14,13,(Color){200,230,140,255}); DrawText(TextFormat("C4  %d",P.bombs),(int)wx+160,(int)wy+34,13,(Color){255,160,90,255});
    DrawText(TextFormat("RANK %d",P.rank),(int)wx+160,(int)wy+56,13,COL_GOLD);
    }
    /* health */
    float hx=uiW/2-150, hy=VIRT_H-44; Bar(hx,hy,300,22,P.hp/P.maxHp,P.hp<P.maxHp*0.3f?COL_RED:COL_GREEN);
    TextC(TextFormat("%d / %d",(int)ceilf(P.hp),(int)P.maxHp),uiW/2,hy+3,15,COL_TEXT);
    if(!P.inVeh&&CityNearVehicle()) TextC(UiTouchMode()?"TAP USE TO ENTER VEHICLE":"[F]  ENTER VEHICLE",uiW/2,VIRT_H*0.66f,20,COL_GOLD);
    if(P.inVeh) TextC(UiTouchMode()?"USE = EXIT   BRAKE = HANDBRAKE":"[F] EXIT    [SPACE] HANDBRAKE    W/S GAS+BRAKE    A/D STEER",uiW/2,VIRT_H-76,15,COL_TEXT);
    if(!P.inVeh&&c4Active()) TextC(UiTouchMode()?"TAP BOOM TO DETONATE":"PRESS V TO DETONATE C4",uiW/2,hy-26,16,COL_GOLD);
    /* feed + banners */
    float fy=UiTouchMode()?136:136;
    for(int i=0;i<5;i++) if(G.feedT[i]>0){ Color c=G.feedC[i]; c.a=(unsigned char)(255*Clampf(G.feedT[i],0,1)); TextR(G.feed[i],uiW-18,fy+i*20,16,c); }
    if(G.bannerT>0){ Color c=G.bannerC; c.a=(unsigned char)(255*Clampf(G.bannerT*2,0,1)); int sz=26; int tw=MeasureText(G.banner,sz); DrawRectangle((int)(uiW/2-tw/2-16),(int)156,tw+32,40,(Color){0,0,0,(unsigned char)(120*Clampf(G.bannerT*2,0,1))}); TextC(G.banner,uiW/2,162,sz,c); }
    if(G.warnGren>0&&fmodf((float)GetTime()*6,1.0f)<0.6f) TextC("! GRENADE !",uiW/2,VIRT_H*0.62f,30,COL_RED);
    if(SV.st.showFps) DrawText(TextFormat("%d FPS",GetFPS()),(int)uiW-90,(int)VIRT_H-24,14,COL_DIM);
    if(UiTouchMode()) DrawTouch();
    else DrawText("[P / ESC] PAUSE   [M] MAP   [F] VEHICLE   [G] GRENADE   [B] C4   [V] DETONATE",16,(int)VIRT_H-22,12,(Color){255,255,255,90});
    if(G.mapBig) { BigMap(); if(uiTapped) G.mapBig=false; }
}
