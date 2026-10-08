#include "../src/game.c"
#include <assert.h>
#include <time.h>
extern int stubKey;
static int Count(void){ int n=0; for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active) n++; return n; }
static void CheckFinite(const char *w){ if(!isfinite(P.pos.x)||!isfinite(P.pos.y)||!isfinite(P.pos.z)||!isfinite(CAM.position.x)){ printf("NaN in %s\n",w); exit(1);} 
  for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active&&(!isfinite(EN[i].pos.x)||!isfinite(EN[i].pos.z))){ printf("enemy NaN %s\n",w); exit(1);} }
int main(void){ setvbuf(stdout,NULL,_IONBF,0);
    SetRandomSeed(7); SaveDefaults(); WorldInit(); GameInit(); AudioInitAll();
    int tests[]={0,1,9,10,19,20,29};
    for(unsigned t=0;t<sizeof(tests)/sizeof(tests[0]);t++){
        int idx=tests[t]; SV.selChar=(t%NUM_CHARS); SV.completed=idx; 
        GameStartMission(idx);
        printf("mission %d: obs=%d deco=%d barrels=%d total=%d rounds=%d spawned=%d active=%d base=(%d,%d) %s start=(%.0f,%.0f)\n",idx+1,nObs,nDeco,nBarrel,M.total,M.rounds,M.spawned,Count(),BASE.br,BASE.bc,M.district,M.start.x,M.start.z);
        assert(nObs<MAX_OBS-5); assert(nDeco<MAX_DECO-5); assert(M.total>=10); assert(Count()==M.spawned);
        assert(WorldFree(M.start.x,M.start.z,0.45f));
        for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active&&!EN[i].isStatic) assert(WorldFree(EN[i].pos.x,EN[i].pos.z,0.5f));
        /* phase 1: wander toward base shooting randomly, at invulnerable HP so we test long running logic */
        for(int f=0;f<1800;f++){
            memset(&IN,0,sizeof(IN)); IN.move_y=1; IN.move_x=(f/120%2)?0.5f:-0.5f; IN.look_x=0.01f*sinf(f*0.05f); IN.sprint=(f%300<150);
            IN.fire=(f%7<4); IN.firePressed=(f%7==0); IN.aim=(f%400>300); IN.reload=(f%500==0); IN.grenade=(f%200==0); IN.jump=(f%90==0); IN.swap=(f%650==0); IN.c4=(f%700==5); IN.det=(f%700==300);
            if(f%30==0){ Vector3 d=V3Norm(V3Sub(BASE.center,P.pos)); G.camYaw=atan2f(d.x,d.z); }
            P.maxHp=1e6f; P.hp=1e6f; P.alive=true; if(P.bombs<2)P.bombs=2; if(P.gren<3)P.gren=3;
            GameUpdate(1/60.0f); GameDraw3D(); CheckFinite("phase1");
            if(G.screen==SCR_OVER||G.screen==SCR_COMPLETE){ printf("  ended early screen=%d\n",G.screen); break; }
        }
        printf("  after wander: stage=%d kills=%d alive=%d dist_base=%.0f\n",M.stage,P.kills,GameAlive(),V3DistXZ(P.pos,BASE.center));
        /* phase 2: cheat through rounds */
        P.pos=V3(BASE.center.x,0,BASE.center.z+60); G.screen=SCR_GAME; P.alive=true; M.stage=(M.stage<1)?1:M.stage; int guard=0;
        while(M.stage<2&&guard++<20000){ for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active&&!EN[i].dead) DamageEnemy(i,9999,true,V3(1,0,0),true,false);
            memset(&IN,0,sizeof(IN)); P.alive=true; P.hp=P.maxHp; GameUpdate(1/60.0f); CheckFinite("phase2"); }
        printf("  stage=%d round=%d/%d killed=%d/%d guard=%d\n",M.stage,M.round,M.rounds,M.killed,M.total,guard);
        assert(M.stage==2); assert(M.killed==M.total); assert(M.round==M.rounds);
        /* phase 3: plant C4 on the cache and detonate */
        P.pos=V3(BASE.cache.x+3,0,BASE.cache.z+6); P.bombs=2; P.hp=P.maxHp*10; P.maxHp=P.hp;
        memset(&IN,0,sizeof(IN)); IN.c4=true; GameUpdate(1/60.0f); memset(&IN,0,sizeof(IN)); IN.det=true; GameUpdate(1/60.0f); memset(&IN,0,sizeof(IN));
        for(int f=0;f<400&&G.screen==SCR_GAME;f++){ GameUpdate(1/60.0f); }
        printf("  after C4: stage=%d cacheHP=%.0f screen=%d completed=%d stars=%d newchar=%d\n",M.stage,M.cacheHP,G.screen,SV.completed,G.rStars,G.rNewChar);
        assert(G.screen==SCR_COMPLETE); assert(SV.completed>=idx+1);
    }
    /* enemy AI: put a lone enemy out of LOS and check he hunts via the flow field */
    GameStartMission(5); for(int i=0;i<MAX_ENEMY;i++) EN[i].active=false;
    P.pos=V3(M.start.x,0,M.start.z); Vector3 sp=V3(M.start.x+60,0,M.start.z); int tries=0; while(!WorldFree(sp.x,sp.z,1)&&tries++<50) sp.z+=7;
    SpawnEnemy(EN_RIFLE,sp,true,false); float d0=V3DistXZ(EN[0].pos,P.pos);
    for(int f=0;f<600;f++){ memset(&IN,0,sizeof(IN)); P.hp=P.maxHp; GameUpdate(1/60.0f); CheckFinite("ai"); }
    printf("AI: dist %.1f -> %.1f (canSee=%d state=%d)\n",d0,V3DistXZ(EN[0].pos,P.pos),EN[0].canSee,EN[0].state);
    /* lethality calibration: 5 alerted riflemen, stationary player on open road, normal difficulty */
    for(int trial=0;trial<3;trial++){
        SV.st.difficulty=1; GameStartMission(4); for(int i=0;i<MAX_ENEMY;i++) EN[i].active=false;
        P.pos=V3(M.start.x,0,M.start.z); P.alive=true; M.stage=1;
        int placed=0; for(int k=0;k<40&&placed<5;k++){ Vector3 p=V3(P.pos.x+RndR(-30,30),0,P.pos.z+RndR(-30,30)); if(V3DistXZ(p,P.pos)>18&&WorldFree(p.x,p.z,1)){ SpawnEnemy(EN_RIFLE,p,true,false); placed++; } }
        float t=0; while(P.alive&&t<60){ memset(&IN,0,sizeof(IN)); GameUpdate(1/60.0f); t+=1/60.0f; }
        printf("lethality trial %d: placed=%d player %s after %.1fs (hp=%.0f)\n",trial,placed,P.alive?"SURVIVED":"died",t,P.hp);
    }

    /* ===== city life: free roam, citizens, traffic, driving ===== */
    for(int hi=0;hi<3;hi++){
        SV.st.drawDist=hi; SV.selChar=0; GameStartMission(-1); assert(M.free);
        int nc=0,nv=0; for(int i=0;i<MAX_CIT;i++) if(CIT[i].active) nc++; for(int i=0;i<MAX_VEH;i++) if(VEH[i].active) nv++;
        printf("free roam draw=%d: citizens=%d vehicles=%d stalls=%d obs=%d deco=%d start=(%.0f,%.0f) %s\n",hi,nc,nv,nStall,nObs,nDeco,M.start.x,M.start.z,DistrictAt(M.start.x,M.start.z));
        assert(nc>10&&nv>3); assert(nObs<MAX_OBS-10&&nDeco<MAX_DECO-10);
        clock_t t0=clock(); int frames=900;
        for(int f=0;f<frames;f++){ memset(&IN,0,sizeof(IN)); IN.move_y=1; IN.move_x=sinf(f*0.03f)*0.6f; IN.sprint=(f%400<200); IN.look_x=0.004f; P.hp=P.maxHp;
            GameUpdate(1/60.0f); GameDraw3D(); CheckFinite("freeroam"); }
        double ms=1000.0*(clock()-t0)/CLOCKS_PER_SEC/frames;
        printf("  walk-sim CPU: %.2f ms/frame (logic+draw submit, no GPU)\n",ms);
        for(int i=0;i<MAX_CIT;i++) if(CIT[i].active&&(!isfinite(CIT[i].pos.x)||!isfinite(CIT[i].pos.z))){ printf("citizen NaN\n"); return 1; }
    }
    SV.st.drawDist=1; GameStartMission(-1);
    /* vendors exist? */ { int vend=0; for(int i=0;i<MAX_CIT;i++) if(CIT[i].active&&CIT[i].vendor) vend++; printf("vendors standing at stalls: %d\n",vend); }
    /* hijack a moving vehicle */
    { int vi=-1; for(int i=0;i<MAX_VEH;i++) if(VEH[i].active&&!VEH[i].parked){ vi=i; break; } assert(vi>=0);
      P.pos=V3Add(VEH[vi].pos,V3(2.5f,0,0)); memset(&IN,0,sizeof(IN)); IN.use=true; GameUpdate(1/60.0f); assert(P.inVeh); printf("hijacked vehicle type %d\n",VEH[P.vehIdx].type);
      int viol=0; float maxSpeed=0; int impacts=0; float hp0=VEH[P.vehIdx].hp;
      for(int f=0;f<3600&&P.inVeh;f++){ memset(&IN,0,sizeof(IN)); IN.move_y=(f%900<600)?1:-0.6f; IN.move_x=sinf(f*0.02f); IN.brake=(f%700>650); P.hp=P.maxHp; VEH[P.vehIdx].hp=fmaxf(VEH[P.vehIdx].hp,50);
          GameUpdate(1/60.0f); GameDraw3D(); CheckFinite("drive"); Vehicle *v=&VEH[P.vehIdx]; if(fabsf(v->speed)>maxSpeed) maxSpeed=fabsf(v->speed);
          if(!WorldFree(v->pos.x,v->pos.z,0.2f)) viol++; if(v->hp<hp0-1){ impacts++; hp0=v->hp; } }
      printf("  drove 60s: maxSpeed=%.1f m/s (%.0f km/h) inside-obstacle frames=%d impacts=%d pos=(%.0f,%.0f)\n",maxSpeed,maxSpeed*3.6f,viol,impacts,P.pos.x,P.pos.z);
      assert(maxSpeed>5); assert(viol<30);
      /* run over a citizen */
      Vehicle *v=&VEH[P.vehIdx]; v->speed=15; int ci=-1; for(int i=0;i<MAX_CIT;i++) if(!CIT[i].active){ ci=i; break; }
      memset(&CIT[ci],0,sizeof(Citizen)); CIT[ci].active=true; CIT[ci].hp=30; CIT[ci].vendor=true; CIT[ci].stall=-1; CIT[ci].pos=V3Add(v->pos,V3Mul(YawDir(v->yaw),2.0f));
      int ck0=P.civKills; memset(&IN,0,sizeof(IN)); IN.move_y=1; GameUpdate(1/60.0f); printf("  run-over: civKills %d -> %d, score=%d\n",ck0,P.civKills,P.score); assert(P.civKills==ck0+1);
      /* exit */ memset(&IN,0,sizeof(IN)); IN.use=true; GameUpdate(1/60.0f); assert(!P.inVeh);
      /* shoot a citizen with the real fire path */
      memset(&CIT[ci],0,sizeof(Citizen)); CIT[ci].active=true; CIT[ci].hp=30; CIT[ci].vendor=true; CIT[ci].stall=-1; P.pos=V3(1200,0,1200); WorldFree(1,1,1);
      CityReset(); for(int i=0;i<MAX_CIT;i++) CIT[i].active=false; for(int i=0;i<MAX_VEH;i++) VEH[i].active=false;
      Vector3 f2=V3(sinf(0.3f),0,cosf(0.3f)); G.camYaw=0.3f; G.camPitch=0; P.yaw=0.3f; CIT[ci].active=true; CIT[ci].hp=20; CIT[ci].vendor=true; CIT[ci].stall=-1; CIT[ci].pos=V3Add(P.pos,V3Mul(f2,12)); 
      GameUpdate(1/60.0f); CAM.position=V3(P.pos.x,P.pos.y+1.6f,P.pos.z); int before=P.civKills;
      for(int k=0;k<12&&P.civKills==before;k++){ memset(&IN,0,sizeof(IN)); IN.fire=true; IN.firePressed=true; P.fireT=0; P.mag[P.cw]=30; CIT[ci].hp=20; CIT[ci].pos=V3Add(P.pos,V3Mul(f2,12)); GameUpdate(1/60.0f); }
      printf("  shot citizen: civKills %d -> %d\n",before,P.civKills); assert(P.civKills>before);
    }
    /* destroy a car by explosion with the player inside -> wreck + ejection */
    { GameStartMission(-1); int vi=-1; for(int i=0;i<MAX_VEH;i++) if(VEH[i].active){ vi=i; break; } P.pos=V3Add(VEH[vi].pos,V3(2,0,0)); memset(&IN,0,sizeof(IN)); IN.use=true; GameUpdate(1/60.0f); assert(P.inVeh);
      memset(&IN,0,sizeof(IN)); VEH[vi].hp=1; GameExplode(V3(VEH[vi].pos.x+1,0.5f,VEH[vi].pos.z),9,400,false); for(int f=0;f<10;f++){ GameUpdate(1/60.0f); }
      printf("vehicle explosion: wreck=%d playerInVeh=%d hp=%.0f/%.0f alive=%d\n",VEH[vi].wreck,P.inVeh,P.hp,P.maxHp,P.alive); assert(VEH[vi].wreck&&!P.inVeh); }
    /* parked static car can be entered */
    { GameStartMission(-1); int oi=-1; for(int i=0;i<nObs;i++) if(OBS[i].kind==OB_VEH&&OBS[i].active){ oi=i; break; } assert(oi>=0);
      P.pos=V3((OBS[oi].mn.x+OBS[oi].mx.x)/2+((OBS[oi].mx.x-OBS[oi].mn.x)/2+1.2f),0,(OBS[oi].mn.z+OBS[oi].mx.z)/2); memset(&IN,0,sizeof(IN)); IN.use=true; GameUpdate(1/60.0f);
      printf("parked car enter: inVeh=%d\n",P.inVeh); assert(P.inVeh); }

    /* ===== pause key regression (P / ESC used to resume in the same frame it paused) ===== */
    for(int pass=0;pass<2;pass++){
        int key=pass?KEY_ESCAPE:KEY_P; GameStartMission(-1); G.screen=SCR_GAME; stubKey=0; UiFrame(1/60.0f);
        stubKey=key; UiFrame(1/60.0f); assert(G.screen==SCR_PAUSE); UiDraw(); assert(G.screen==SCR_PAUSE);
        stubKey=0; UiFrame(1/60.0f); UiDraw(); assert(G.screen==SCR_PAUSE);
        stubKey=key; UiFrame(1/60.0f); assert(G.screen==SCR_GAME); stubKey=0; UiFrame(1/60.0f); assert(G.screen==SCR_GAME);
        printf("pause via %s: pause + resume OK\n",pass?"ESC":"P");
    }
    /* ===== weapons: every weapon fires; launcher kills; swap pickups; armor; adrenaline ===== */
    GameStartMission(3); for(int i=0;i<MAX_ENEMY;i++) EN[i].active=false; memset(CIT,0,sizeof(CIT)); memset(VEH,0,sizeof(VEH));
    for(int w=0;w<NUM_WEAPONS;w++){
        P.pos=V3(M.start.x,0,M.start.z); P.alive=true; P.maxHp=1e6f; P.hp=1e6f; P.wid[0]=w; P.mag[0]=WEAPONS[w].mag; P.res[0]=WEAPONS[w].reserve; P.cw=0; P.reloading=false; P.fireT=0;
        G.camYaw=0; G.camPitch=0; CAM.position=V3(P.pos.x,1.6f,P.pos.z-3); SpawnEnemy(EN_RIFLE,V3(P.pos.x,0,P.pos.z+25),false,true); int ei=-1; for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active) ei=i; EN[ei].maxHp=EN[ei].hp=100000;
        float hp0=EN[ei].hp; int shots=0;
        for(int f=0;f<240;f++){ memset(&IN,0,sizeof(IN)); IN.fire=true; IN.firePressed=(f%10==0); P.hp=1e6f; int m0=P.mag[0]; GameUpdate(1/60.0f); if(P.mag[0]<m0) shots++; EN[ei].hp=fminf(EN[ei].hp,EN[ei].hp); P.alive=true; CheckFinite("weapon"); }
        printf("weapon %-14s shots=%3d  damage dealt to target=%.0f\n",WEAPONS[w].name,shots,hp0-EN[ei].hp);
        assert(shots>0); EN[ei].active=false;
    }
    { /* launcher kill */ P.pos=V3(M.start.x,0,M.start.z); P.wid[0]=W_M79; P.mag[0]=1; P.res[0]=9; P.cw=0; P.fireT=0; P.reloading=false; G.camYaw=0; G.camPitch=0; CAM.position=V3(P.pos.x,1.6f,P.pos.z-3);
      for(int i=0;i<MAX_GREN;i++) GR[i].active=false;
      SpawnEnemy(EN_RIFLE,V3(P.pos.x,0,P.pos.z+22),false,true); int ei=-1; for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active) ei=i; EN[ei].hp=EN[ei].maxHp=60;
      memset(&IN,0,sizeof(IN)); IN.fire=true; IN.firePressed=true; P.hp=P.maxHp=1000; GameUpdate(1/60.0f); memset(&IN,0,sizeof(IN)); for(int f=0;f<240&&!EN[ei].dead;f++){ P.hp=1000; GameUpdate(1/60.0f); }
      printf("M79 launcher: target dead=%d hp=%.0f\n",EN[ei].dead,EN[ei].hp); assert(EN[ei].dead); }
    { /* weapon pickup swap + armor + adrenaline */ P.pos=V3(M.start.x,0,M.start.z+3); P.wid[0]=W_AK47; P.cw=0; SpawnPickup(P.pos,4,W_AWM,0); memset(&IN,0,sizeof(IN)); GameUpdate(1/60.0f);
      printf("pickup swap: now holding %s\n",WEAPONS[P.wid[P.cw]].name); assert(P.wid[P.cw]==W_AWM);
      bool dropped=false; for(int i=0;i<MAX_PICKUP;i++) if(PK[i].active&&PK[i].type==4&&PK[i].wid==W_AK47) dropped=true; assert(dropped);
      for(int f=0;f<60;f++) GameUpdate(1/60.0f); assert(P.wid[P.cw]==W_AWM);   /* delay prevents instant re-swap */
      P.alive=true; P.maxHp=100; P.hp=100; P.armor=50; P.adren=0; HurtPlayer(10,V3(0,0,0)); printf("armor test: hp=%.1f armor=%.1f\n",P.hp,P.armor); assert(fabsf(P.hp-96)<0.01f&&fabsf(P.armor-44)<0.01f);
      P.armor=0; P.hp=100; P.adren=5; HurtPlayer(10,V3(0,0,0)); assert(fabsf(P.hp-93)<0.01f);
      SpawnPickup(P.pos,6,0,0); SpawnPickup(P.pos,7,0,0); SpawnPickup(P.pos,5,0,0); int sc=P.score; P.adren=0; P.armor=0; GameUpdate(1/60.0f); printf("adrenaline=%.1f armor=%.0f score+%d\n",P.adren,P.armor,P.score-sc); assert(P.adren>10&&P.armor>=50&&P.score-sc==300); }
    { /* detection system */ GameStartMission(3); for(int i=0;i<MAX_ENEMY;i++) EN[i].active=false; memset(CIT,0,sizeof(CIT)); memset(VEH,0,sizeof(VEH));
      P.pos=V3(M.start.x,0,M.start.z); P.hp=P.maxHp=1e6f; SpawnEnemy(EN_RIFLE,V3(P.pos.x,0,P.pos.z+30),false,true); int ei=-1; for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active) ei=i; EN[ei].yaw=PI;
      float tSus=-1,tDet=-1; int maxState=0;
      for(int f=0;f<1200&&tDet<0;f++){ memset(&IN,0,sizeof(IN)); P.hp=1e6f; GameUpdate(1/60.0f); if(EN[ei].spot>0.3f&&tSus<0) tSus=f/60.0f; if(EN[ei].state==1&&tDet<0) tDet=f/60.0f; if(G.detState>maxState) maxState=G.detState; }
      printf("detection: suspicious at %.1fs, detected at %.1fs, HUD state reached %d\n",tSus,tDet,maxState); assert(tSus>=0&&tDet>tSus&&maxState>=1);
      /* stealth kill */ for(int i=0;i<MAX_ENEMY;i++) EN[i].active=false; SpawnEnemy(EN_RIFLE,V3(P.pos.x+40,0,P.pos.z),false,true); ei=-1; for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active) ei=i; EN[ei].spot=0; int sc=P.score;
      DamageEnemy(ei,9999,true,V3(1,0,0),true,false); printf("stealth kill score gain=%d\n",P.score-sc); assert(P.score-sc>=250); }
    { /* driving camera: smooth, no flip while reversing */ GameStartMission(-1); int vi=-1; for(int i=0;i<MAX_VEH;i++) if(VEH[i].active&&!VEH[i].parked){ vi=i; break; } P.pos=V3Add(VEH[vi].pos,V3(2.5f,0,0)); memset(&IN,0,sizeof(IN)); IN.use=true; GameUpdate(1/60.0f); assert(P.inVeh);
      Vector3 prev=CAM.position; float maxStep=0,sumLag=0; int n=0; float vstep=0;
      for(int f=0;f<1500;f++){ memset(&IN,0,sizeof(IN)); IN.move_y=1; IN.move_x=sinf(f*0.04f); P.hp=P.maxHp; VEH[P.vehIdx].hp=VEH[P.vehIdx].maxHp; GameUpdate(1/60.0f); CheckFinite("cam");
          if(f>120){ float st=V3Dist(CAM.position,prev); if(st>maxStep) maxStep=st; sumLag+=fabsf(AngleDiff(G.camYaw,VEH[P.vehIdx].yaw)); n++; } prev=CAM.position; (void)vstep; }
      printf("drive cam: max camera step/frame=%.2f m, avg yaw lag=%.2f rad (%.0f deg)\n",maxStep,sumLag/n,sumLag/n*57.3f); assert(maxStep<2.0f&&sumLag/n<0.8f);
      for(int f=0;f<300;f++){ memset(&IN,0,sizeof(IN)); IN.move_y=-1; P.hp=P.maxHp; VEH[P.vehIdx].hp=VEH[P.vehIdx].maxHp; GameUpdate(1/60.0f); }
      float rl=fabsf(AngleDiff(G.camYaw,VEH[P.vehIdx].yaw)); printf("reverse: camera yaw vs car yaw diff=%.2f rad, speed=%.1f\n",rl,VEH[P.vehIdx].speed); assert(rl<1.0f); }
    { /* crime: kill a civilian -> police cars chase and officers fight; on-foot vehicle hit hurts */
        GameStartMission(-1); memset(&IN,0,sizeof(IN)); for(int i=0;i<MAX_ENEMY;i++) EN[i].active=false;
        int ci=-1; for(int i=0;i<MAX_CIT;i++) if(CIT[i].active&&!CIT[i].dead){ ci=i; break; } assert(ci>=0);
        CityDamage(1,ci,999,V3(1,0,0),true); assert(gWanted==1&&CIT[ci].dead);
        int pol=0,cops=0; for(int f=0;f<60*70;f++){ GameUpdate(1/60.0f); P.hp=P.maxHp; if(f==60*40){ for(int i=0;i<MAX_VEH;i++) if(VEH[i].active&&VEH[i].police) pol++; } }
        for(int i=0;i<MAX_ENEMY;i++) if(EN[i].active&&EN[i].cop) cops++;
        printf("crime: wanted=%d police cars seen=%d cops spawned=%d\n",gWanted,pol,cops);
        assert(pol>0||cops>0);
        int mk=M.killed; GameSpawnCop(V3Add(P.pos,V3(5,0,0))); for(int i=0;i<MAX_ENEMY;i++) if(EN[i].cop&&EN[i].active&&!EN[i].dead){ GameHurtEnemy(i,9999,false,V3(1,0,0),true,false); break; }
        assert(M.killed==mk); assert(GameAlive()==0);
        float hp0=P.hp; Vector3 pp=P.pos; GameHurtPlayer(10,pp); assert(P.hp<hp0);
        printf("accident damage + cop-kill not counted as mission hostile OK\n");
    }
    printf("ALL OK\n"); return 0;
}
