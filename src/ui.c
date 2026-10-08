/* UI framework, input, authentication and all menu screens */
#include "ui_internal.h"
#include <ctype.h>

float uiScale=1,uiW=1280; bool gQuit=false;
Ptr uiPtr[10]; int uiNPtr=0; bool uiTapped=false; Vector2 uiTapPos,uiMouse;
#define R4(x,y,w,h) ((Rectangle){(x),(y),(w),(h)})
static bool hasProfile=false; static int deployPending=0, settingsBack=SCR_MENU; static float splashT=0;

bool UiTouchMode(void){ return SV.st.touch; }

/* ---------- pointers & widgets ---------- */
static void Poll(void){
    float sw=(float)GetScreenWidth(),sh=(float)GetScreenHeight(); uiScale=sh/VIRT_H; uiW=sw/uiScale;
    uiMouse=(Vector2){GetMouseX()/uiScale,GetMouseY()/uiScale};
    static int prev[10]; static int nprev=0;
    uiNPtr=0; int tc=GetTouchPointCount(); if(tc>10) tc=10;
    for(int i=0;i<tc;i++){ Vector2 p=GetTouchPosition(i); uiPtr[uiNPtr++]=(Ptr){GetTouchPointId(i),(Vector2){p.x/uiScale,p.y/uiScale},false}; }
    if(uiNPtr==0&&IsMouseButtonDown(MOUSE_BUTTON_LEFT)) uiPtr[uiNPtr++]=(Ptr){1000,uiMouse,false};
    uiTapped=false;
    for(int i=0;i<uiNPtr;i++){ bool known=false; for(int k=0;k<nprev;k++) if(prev[k]==uiPtr[i].id) known=true;
        uiPtr[i].pressed=!known; if(!known&&!uiTapped){ uiTapped=true; uiTapPos=uiPtr[i].pos; } }
    nprev=uiNPtr; for(int i=0;i<uiNPtr;i++) prev[i]=uiPtr[i].id;
}
static bool PtIn(Vector2 p,Rectangle r){ return CheckCollisionPointRec(p,r); }
void TextC(const char *t,float cx,float y,int sz,Color c){ int w=MeasureText(t,sz); DrawText(t,(int)(cx-w/2.0f),(int)y,sz,c); }
void TextR(const char *t,float rx,float y,int sz,Color c){ int w=MeasureText(t,sz); DrawText(t,(int)(rx-w),(int)y,sz,c); }
void Panel(Rectangle r,Color c){ DrawRectangleRounded(r,0.05f,8,c); DrawRectangleLinesEx(r,1,(Color){70,92,104,255}); }
void Tri(Vector2 a,Vector2 b,Vector2 c,Color col){
    float cr=(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x);
    if(cr<0) DrawTriangle(a,b,c,col); else DrawTriangle(a,c,b,col);
}
bool Btn(Rectangle r,const char *label,int style){
    bool hover=!UiTouchMode()&&PtIn(uiMouse,r), down=false;
    for(int i=0;i<uiNPtr;i++) if(PtIn(uiPtr[i].pos,r)) down=true;
    Color base=(style==0)?COL_GREEN:(style==2)?COL_RED:(Color){34,52,64,255};
    Color c=down?ColMul(base,0.7f):(hover?ColMul(base,1.25f):base);
    DrawRectangleRounded(r,0.25f,8,c); DrawRectangleLinesEx(r,1,(Color){255,255,255,(unsigned char)(hover?150:60)});
    int sz=(int)Clampf(r.height*0.42f,12,26); TextC(label,r.x+r.width/2,r.y+(r.height-sz)/2,sz,COL_TEXT);
    if(uiTapped&&PtIn(uiTapPos,r)){ uiTapped=false; Sfx(SFX_CLICK,0.7f,1.0f); return true; }
    return false;
}
static bool Toggle(Rectangle r,const char *label,bool *v){
    char t[64]; snprintf(t,sizeof(t),"%s: %s",label,*v?"ON":"OFF");
    if(Btn(r,t,*v?0:1)){ *v=!*v; return true; } return false;
}
static bool Cycle(Rectangle r,const char *label,const char **names,int n,int *v){
    char t[64]; snprintf(t,sizeof(t),"%s: %s",label,names[*v]);
    if(Btn(r,t,1)){ *v=(*v+1)%n; return true; } return false;
}
static bool Slider(Rectangle r,const char *label,float *v,float lo,float hi,const char *fmt){
    DrawText(label,(int)r.x,(int)r.y,18,COL_TEXT); TextR(TextFormat(fmt,*v),r.x+r.width,r.y,18,COL_GOLD);
    Rectangle tr=R4(r.x,r.y+30,r.width,10); DrawRectangleRounded(tr,0.5f,6,(Color){30,44,54,255});
    float f=(*v-lo)/(hi-lo); DrawRectangleRounded(R4(tr.x,tr.y,tr.width*f,tr.height),0.5f,6,COL_GREEN); DrawCircle((int)(tr.x+tr.width*f),(int)(tr.y+5),11,COL_TEXT);
    Rectangle hit=R4(r.x-8,r.y+14,r.width+16,40); bool ch=false;
    for(int i=0;i<uiNPtr;i++) if(PtIn(uiPtr[i].pos,hit)||(uiPtr[i].id==1000&&PtIn(uiPtr[i].pos,R4(hit.x-30,hit.y-6,hit.width+60,hit.height+12)))){
        float nv=lo+Clampf((uiPtr[i].pos.x-tr.x)/tr.width,0,1)*(hi-lo); if(fabsf(nv-*v)>1e-4f){ *v=nv; ch=true; } }
    return ch;
}
static void Star(float cx,float cy,float r,Color col){
    Vector2 p[10]; for(int i=0;i<10;i++){ float a=-PI/2+i*PI/5, rr=(i%2==0)?r:r*0.42f; p[i]=(Vector2){cx+cosf(a)*rr,cy+sinf(a)*rr}; }
    for(int i=0;i<10;i++) Tri((Vector2){cx,cy},p[i],p[(i+1)%10],col);
}
static void Begin2D(void){ Camera2D c={{0,0},{0,0},0,uiScale}; BeginMode2D(c); }
static void Background(void){
    float sw=(float)GetScreenWidth(),sh=(float)GetScreenHeight();
    DrawRectangleGradientV(0,0,(int)sw,(int)sh,(Color){10,26,30,255},(Color){4,8,12,255});
    DrawCircle((int)(sw*0.82f),(int)(sh*0.5f),sh*0.42f,(Color){244,42,65,14}); DrawCircle((int)(sw*0.12f),(int)(sh*0.9f),sh*0.5f,(Color){0,166,118,14});
}
static void Title(const char *t,const char *sub){
    DrawText(t,40,26,40,COL_TEXT); if(sub) DrawText(sub,42,72,16,COL_DIM); DrawRectangle(40,98,90,3,COL_GREEN); DrawRectangle(130,98,40,3,COL_RED);
}

/* ---------- 3D backdrops ---------- */
static void SkyBackdrop(float pitch,float fov){
    float sw=(float)GetScreenWidth(),sh=(float)GetScreenHeight();
    float hy=sh*0.5f+(tanf(pitch)/tanf(fov*0.5f*DEG2RAD))*sh*0.5f; hy=Clampf(hy,0,sh);
    DrawRectangleGradientV(0,0,(int)sw,(int)hy+1,(Color){88,120,172,255},(Color){232,178,132,255});
    DrawRectangle(0,(int)hy,(int)sw,(int)(sh-hy)+1,(Color){222,172,130,255});
}
static void OrbitBackdrop(void){
    float t=(float)GetTime(), a=t*0.05f;
    Camera3D c={{800+cosf(a)*430,190+22*sinf(t*0.3f),800+sinf(a)*430},{800,10,800},{0,1,0},55,CAMERA_PERSPECTIVE};
    SkyBackdrop(atan2f(c.target.y-c.position.y,430),55);
    Begin3D(c); WorldDraw(c.position,V3Norm(V3(c.target.x-c.position.x,0,c.target.z-c.position.z)),1,t); End3D();
    DrawRectangleGradientH(0,0,GetScreenWidth(),GetScreenHeight(),(Color){4,8,12,215},(Color){4,8,12,70});
}
static void DrawGameScene(void){
    SkyBackdrop(G.camPitch,CAM.fovy);
    Begin3D(CAM); GameDraw3D(); End3D();
    Begin2D(); HudDraw(); EndMode2D();
}

/* ---------- text input ---------- */
typedef struct { char buf[64]; const char *label; bool secret; int max; } Field;
static Field F[5]; static int activeF=0; static bool showPw=false,kbOn=false,kbShift=false;
static char errMsg[96]="", infoMsg[128]=""; static int fails=0; static float lockT=0,resetArm=0,bsT=0;
static void FieldsClear(void){ memset(F,0,sizeof(F)); activeF=0; errMsg[0]=0; infoMsg[0]=0; }
static void TypeChar(Field *f,int ch){ size_t n=strlen(f->buf); if((int)n<f->max&&ch>=32&&ch<=126){ f->buf[n]=(char)ch; f->buf[n+1]=0; } }
static void Backspace(Field *f){ size_t n=strlen(f->buf); if(n>0) f->buf[n-1]=0; }
static bool PhysicalInput(int nf){
    int ch; while((ch=GetCharPressed())>0) TypeChar(&F[activeF],ch);
    if(IsKeyPressed(KEY_BACKSPACE)){ Backspace(&F[activeF]); bsT=-0.35f; }
    else if(IsKeyDown(KEY_BACKSPACE)){ bsT+=GetFrameTime(); if(bsT>0.04f){ Backspace(&F[activeF]); bsT=0; } }
    if(IsKeyPressed(KEY_TAB)||IsKeyPressed(KEY_DOWN)) activeF=(activeF+1)%nf;
    if(IsKeyPressed(KEY_UP)) activeF=(activeF+nf-1)%nf;
    return IsKeyPressed(KEY_ENTER)||IsKeyPressed(KEY_KP_ENTER);
}
static void DrawField(int i,Rectangle r){
    Field *f=&F[i]; DrawText(f->label,(int)r.x,(int)r.y-20,15,COL_DIM);
    bool act=(activeF==i); DrawRectangleRounded(r,0.18f,8,(Color){8,14,19,255}); DrawRectangleLinesEx(r,act?2:1,act?COL_GREEN:(Color){70,92,104,255});
    char shown[72]; if(f->secret&&!showPw){ int n=(int)strlen(f->buf); for(int k=0;k<n&&k<70;k++) shown[k]='*'; shown[n<70?n:70]=0; } else snprintf(shown,sizeof(shown),"%s",f->buf);
    int tw=MeasureText(shown,22); DrawText(shown,(int)r.x+12,(int)(r.y+r.height/2-11),22,COL_TEXT);
    if(act&&fmodf((float)GetTime(),1.0f)<0.55f) DrawRectangle((int)r.x+14+tw,(int)(r.y+8),2,(int)r.height-16,COL_GREEN);
    if(uiTapped&&PtIn(uiTapPos,r)){ activeF=i; uiTapped=false; }
}
/* on-screen keyboard: returns char, -1 backspace, -2 enter, 0 nothing */
static int Osk(void){
    static const char *rows[4]={"1234567890","qwertyuiop","asdfghjkl@","zxcvbnm_.-"};
    float kw=fminf(88,(uiW-40)/10-6),kh=48,gap=6,tw=10*(kw+gap)-gap,x0=(uiW-tw)/2,y0=VIRT_H-5*(kh+gap)-6; int out=0;
    DrawRectangle(0,(int)y0-8,(int)uiW+1,(int)(VIRT_H-y0+8),(Color){5,9,12,245});
    for(int r=0;r<4;r++) for(int c=0;c<10;c++){
        char ch=rows[r][c]; if(kbShift&&isalpha((unsigned char)ch)) ch=(char)toupper((unsigned char)ch);
        char s[2]={ch,0}; if(Btn(R4(x0+c*(kw+gap),y0+r*(kh+gap),kw,kh),s,1)) out=ch; }
    float u=kw+gap,y=y0+4*(kh+gap),x=x0;
    if(Btn(R4(x,y,u*2-gap,kh),kbShift?"SHIFT*":"SHIFT",kbShift?0:1)) kbShift=!kbShift; x+=u*2;
    if(Btn(R4(x,y,u-gap,kh),"!",1)) out='!'; x+=u;
    if(Btn(R4(x,y,u*3-gap,kh),"SPACE",1)) out=' '; x+=u*3;
    if(Btn(R4(x,y,u-gap,kh),"?",1)) out='?'; x+=u;
    if(Btn(R4(x,y,u*1.5f-gap,kh),"DEL",2)) out=-1; x+=u*1.5f;
    if(Btn(R4(x,y,u*1.5f-gap,kh),"NEXT",0)) out=-2;
    return out;
}
static bool KeyboardToggle(void){ if(Btn(R4(uiW-158,14,142,34),kbOn?"KEYBOARD: ON":"KEYBOARD: OFF",1)){ kbOn=!kbOn; return true; } return false; }
static bool FormInput(int nf){
    bool enter=PhysicalInput(nf);
    if(kbOn){ int k=Osk(); if(k>0) TypeChar(&F[activeF],k); else if(k==-1) Backspace(&F[activeF]); else if(k==-2){ if(activeF<nf-1) activeF++; else enter=true; } }
    return enter;
}

/* ---------- auth screens ---------- */
static bool ValidUser(const char *u){ size_t n=strlen(u); if(n<3||n>20) return false; for(size_t i=0;i<n;i++) if(!isalnum((unsigned char)u[i])&&u[i]!='_') return false; return true; }
static bool ContainsNoCase(const char *h,const char *n){ size_t hl=strlen(h),nl=strlen(n); if(nl==0||nl>hl) return false;
    for(size_t i=0;i+nl<=hl;i++){ size_t k=0; while(k<nl&&tolower((unsigned char)h[i+k])==tolower((unsigned char)n[k])) k++; if(k==nl) return true; } return false; }
static void SetupRegister(void){ FieldsClear(); F[0]=(Field){"","FULL NAME",false,30}; F[1]=(Field){"","USERNAME (letters, numbers, _)",false,20}; F[2]=(Field){"","PASSWORD (min 4)",true,30}; F[3]=(Field){"","CONFIRM PASSWORD",true,30}; F[4]=(Field){"","PASSWORD HINT (shown if you forget)",false,50}; kbOn=UiTouchMode(); G.screen=SCR_REGISTER; }
static void SetupLogin(void){ FieldsClear(); F[0]=(Field){"","USERNAME",false,20}; F[1]=(Field){"","PASSWORD",true,30}; kbOn=UiTouchMode(); G.screen=SCR_LOGIN; fails=0; }
static void ScreenRegister(void){
    Background(); Begin2D();
    TextC("CREATE YOUR OPERATOR PROFILE",uiW/2,18,32,COL_TEXT); TextC("Register once. You will sign in every time you play.",uiW/2,56,15,COL_DIM);
    float lx=uiW/2-330,rx=uiW/2+10,w=320,h=44;
    DrawField(0,R4(lx,100,w,h)); DrawField(1,R4(lx,176,w,h)); DrawField(2,R4(lx,252,w,h)); DrawField(3,R4(rx,100,w,h)); DrawField(4,R4(rx,176,w,h));
    if(Btn(R4(rx,236,w,34),showPw?"HIDE PASSWORDS":"SHOW PASSWORDS",1)) showPw=!showPw;
    if(errMsg[0]) TextC(errMsg,uiW/2,300,17,COL_RED);
    bool enter=FormInput(5); KeyboardToggle();
    if(Btn(R4(uiW/2-160,326,320,52),"CREATE ACCOUNT",0)||enter){
        errMsg[0]=0;
        if(strlen(F[0].buf)<2) snprintf(errMsg,sizeof(errMsg),"Enter your full name.");
        else if(!ValidUser(F[1].buf)) snprintf(errMsg,sizeof(errMsg),"Username: 3-20 letters, numbers or underscore.");
        else if(strlen(F[2].buf)<4) snprintf(errMsg,sizeof(errMsg),"Password must be at least 4 characters.");
        else if(strcmp(F[2].buf,F[3].buf)!=0) snprintf(errMsg,sizeof(errMsg),"Passwords do not match.");
        else if(strlen(F[4].buf)<3) snprintf(errMsg,sizeof(errMsg),"Add a password hint (at least 3 characters).");
        else if(ContainsNoCase(F[4].buf,F[2].buf)) snprintf(errMsg,sizeof(errMsg),"The hint must not contain your password.");
        else{ SaveRegister(F[0].buf,F[1].buf,F[2].buf,F[4].buf); hasProfile=true; SV.selChar=0; SaveWrite(); FieldsClear(); G.screen=SCR_MENU; }
    }
    EndMode2D();
}
static void ScreenLogin(void){
    Background(); Begin2D();
    TextC("SIGN IN",uiW/2,14,38,COL_TEXT); TextC(TextFormat("Welcome back, %s",SV.name),uiW/2,58,16,COL_DIM);
    float x=uiW/2-190,w=380;
    DrawField(0,R4(x,112,w,44)); DrawField(1,R4(x,188,w,44));
    if(Btn(R4(x+w-110,160,110,22),showPw?"HIDE":"SHOW",1)) showPw=!showPw;
    if(errMsg[0]) TextC(errMsg,uiW/2,238,16,COL_RED); if(infoMsg[0]) TextC(infoMsg,uiW/2,238,16,COL_GOLD);
    if(lockT>0) lockT-=GetFrameTime();
    bool enter=FormInput(2); KeyboardToggle();
    if(Btn(R4(x,262,w,50),lockT>0?TextFormat("LOCKED %ds",(int)lockT+1):"LOGIN",0)||enter){
        if(lockT<=0){
            if(SaveCheckLogin(F[0].buf,F[1].buf)){ if(!CharUnlocked(SV.selChar,SV.completed)) SV.selChar=0; FieldsClear(); G.screen=SCR_MENU; }
            else{ fails++; infoMsg[0]=0; snprintf(errMsg,sizeof(errMsg),"Incorrect username or password."); F[1].buf[0]=0;
                if(fails>=3) snprintf(infoMsg,sizeof(infoMsg),"PASSWORD HINT: %s",SV.hint);
                if(fails>=5){ lockT=10; fails=3; } }
        }
    }
    if(Btn(R4(x,320,w,40),"FORGOT PASSWORD? SHOW HINT",1)){ errMsg[0]=0; snprintf(infoMsg,sizeof(infoMsg),"PASSWORD HINT: %s",SV.hint); }
    if(resetArm>0) resetArm-=GetFrameTime();
    if(Btn(R4(x,366,w,34),resetArm>0?"TAP AGAIN: ERASE PROFILE + PROGRESS":"RESET PROFILE",2)){
        if(resetArm>0){ remove(SAVE_FILE); SaveDefaults(); hasProfile=false; SetupRegister(); resetArm=0; } else resetArm=4; }
    EndMode2D();
}

/* ---------- main menu ---------- */
static const char *RankTitle(int c){ static const char *t[8]={"RECRUIT","PRIVATE","CORPORAL","SERGEANT","LIEUTENANT","CAPTAIN","MAJOR","COLONEL"}; static const int th[8]={0,3,6,10,15,20,25,30}; int k=0; for(int i=0;i<8;i++) if(c>=th[i]) k=i; return t[k]; }
static void ScreenMenu(void){
    OrbitBackdrop(); Begin2D();
    DrawText("DHAKA 2099",50,40,64,COL_TEXT); DrawText("URBAN WARFARE",54,106,26,COL_GOLD); DrawRectangle(54,140,120,4,COL_GREEN); DrawRectangle(174,140,50,4,COL_RED);
    float x=54,y=190,w=330,h=52,s=60; int n=0;
    if(Btn(R4(x,y+s*n++,w,h),"PLAY",0)) G.screen=SCR_MISSIONS;
    if(Btn(R4(x,y+s*n++,w,h),"FREE ROAM",1)){ G.selMission=-1; deployPending=2; }
    if(Btn(R4(x,y+s*n++,w,h),"OPERATORS",1)){ G.prevScreen=SCR_MENU; G.screen=SCR_CHARS; }
    if(Btn(R4(x,y+s*n++,w,h),"SETTINGS",1)){ settingsBack=SCR_MENU; G.screen=SCR_SETTINGS; }
    if(Btn(R4(x,y+s*n++,w,h),"HOW TO PLAY",1)) G.screen=SCR_HELP;
    if(Btn(R4(x,y+s*n++,w,h),"CREDITS",1)) G.screen=SCR_CREDITS;
    if(Btn(R4(x,y+s*n++,w,h),"LOG OUT",1)){ SaveWrite(); SetupLogin(); }
#if !defined(PLATFORM_WEB)&&!defined(PLATFORM_ANDROID)
    if(Btn(R4(x,y+s*n++,w,h),"QUIT",2)) gQuit=true;
#endif
    Rectangle pr=R4(uiW-380,40,340,236); Panel(pr,COL_PANEL);
    DrawText(SV.name,(int)pr.x+18,(int)pr.y+14,24,COL_TEXT); DrawText(TextFormat("@%s",SV.username),(int)pr.x+18,(int)pr.y+44,15,COL_DIM);
    DrawText(RankTitle(SV.completed),(int)pr.x+18,(int)pr.y+72,20,COL_GOLD);
    DrawText(TextFormat("Missions   %d / %d",SV.completed,MAX_MISSIONS),(int)pr.x+18,(int)pr.y+108,17,COL_TEXT);
    DrawText(TextFormat("Total kills   %d",SV.totalKills),(int)pr.x+18,(int)pr.y+132,17,COL_TEXT);
    DrawText(TextFormat("Headshots   %d",SV.totalHeadshots),(int)pr.x+18,(int)pr.y+156,17,COL_TEXT);
    DrawText(TextFormat("Score   %d",SV.totalScore),(int)pr.x+18,(int)pr.y+180,17,COL_TEXT);
    DrawText(TextFormat("Operator: %s  (Rank %d)",CHARS[SV.selChar].call,CharRank(SV.completed)),(int)pr.x+18,(int)pr.y+206,15,COL_GREEN);
    DrawText("v" GAME_VERSION,(int)uiW-70,(int)VIRT_H-24,14,COL_DIM);
    EndMode2D();
}

/* ---------- mission select + briefing ---------- */
static void ScreenMissions(void){
    Background(); Begin2D(); Title("SELECT MISSION","Three chapters across Dhaka. Complete missions to unlock operators and ranks.");
    if(Btn(R4(uiW-170,24,130,40),"BACK",1)) G.screen=SCR_MENU;
    int cols=6; float cw=172,ch=94,gap=10,gx=(uiW-(cols*cw+(cols-1)*gap))/2,gy=124;
    static const Color chap[3]={{0,166,118,255},{255,200,60,255},{244,42,65,255}};
    for(int i=0;i<MAX_MISSIONS;i++){
        int c=i%cols,r=i/cols; Rectangle b=R4(gx+c*(cw+gap),gy+r*(ch+gap),cw,ch); bool locked=i>SV.completed;
        bool hover=!UiTouchMode()&&PtIn(uiMouse,b);
        DrawRectangleRounded(b,0.12f,8,locked?(Color){16,22,28,255}:(hover?(Color){40,62,74,255}:(Color){26,40,50,255}));
        DrawRectangle((int)b.x,(int)b.y+6,4,(int)ch-12,locked?(Color){60,70,78,255}:chap[i/10]);
        DrawText(TextFormat("%02d",i+1),(int)b.x+14,(int)b.y+10,28,locked?COL_DIM:COL_TEXT);
        DrawText(locked?"LOCKED":MISSION_NAMES[i],(int)b.x+14,(int)b.y+46,locked?14:13,locked?COL_DIM:COL_TEXT);
        if(!locked) for(int s=0;s<3;s++) Star(b.x+cw-62+s*20,b.y+20,8,s<SV.stars[i]?COL_GOLD:(Color){50,64,72,255});
        if(!locked&&i==SV.completed) DrawText("NEW",(int)b.x+cw-44,(int)b.y+ch-24,13,COL_GREEN);
        if(!locked&&uiTapped&&PtIn(uiTapPos,b)){ uiTapped=false; Sfx(SFX_CLICK,0.7f,1.0f); G.selMission=i; G.screen=SCR_BRIEF; }
    }
    EndMode2D();
}
static void ScreenBrief(void){
    Background(); Begin2D(); int i=G.selMission;
    Title(TextFormat("MISSION %d: %s",i+1,MISSION_NAMES[i]),"Briefing");
    Panel(R4(40,130,uiW-80,330),COL_PANEL);
    DrawText(TextFormat("ENEMY BASE LOCATION:  %s, DHAKA",MissionDistrict(i)),64,150,22,COL_GOLD);
    DrawText(TextFormat("HOSTILES:  %d armed mercenaries across %d rounds",MissionEnemyTotal(i),MissionRounds(i)),64,190,20,COL_TEXT);
    DrawText("OBJECTIVES",64,236,16,COL_DIM);
    DrawText("1.  Cross the city and reach the enemy base. Follow the arrow and your minimap.",84,262,18,COL_TEXT);
    DrawText("2.  Eliminate every hostile. Reinforcements arrive when a round is cleared.",84,292,18,COL_TEXT);
    DrawText("3.  Plant C4 (or throw grenades) to destroy the weapons cache.",84,322,18,COL_TEXT);
    DrawText(TextFormat("OPERATOR:  %s  \"%s\"   |   RANK %d",CHARS[SV.selChar].name,CHARS[SV.selChar].call,CharRank(SV.completed)),64,376,18,COL_GREEN);
    const char *dn[3]={"EASY","NORMAL","HARD"}; DrawText(TextFormat("DIFFICULTY:  %s",dn[SV.st.difficulty]),64,408,18,COL_TEXT);
    if(Btn(R4(uiW/2-360,490,230,60),"BACK",1)) G.screen=SCR_MISSIONS;
    if(Btn(R4(uiW/2-115,490,230,60),"CHANGE OPERATOR",1)){ G.prevScreen=SCR_BRIEF; G.screen=SCR_CHARS; }
    if(Btn(R4(uiW/2+130,490,230,60),"DEPLOY",0)) deployPending=2;
    EndMode2D();
}

/* ---------- operators ---------- */
static void StatBar(float x,float y,float w,const char *l,float f,Color c){
    DrawText(l,(int)x,(int)y,14,COL_DIM); DrawRectangle((int)x+90,(int)y+3,(int)w-90,10,(Color){30,44,54,255}); DrawRectangle((int)x+90,(int)y+3,(int)((w-90)*Clampf(f,0,1)),10,c);
}
static int viewChar=0;
static void ScreenChars(void){
    float t=(float)GetTime();
    DrawRectangleGradientV(0,0,GetScreenWidth(),GetScreenHeight(),(Color){14,32,38,255},(Color){4,8,12,255});
    Camera3D c={{0,1.35f,4.3f},{0.1f,1.0f,0},{0,1,0},38,CAMERA_PERSPECTIVE};
    Begin3D(c); DrawFlat(0,0.01f,0,6,6,(Color){22,34,40,255}); BoxC(V3(0,0.06f,0),V3(2.4f,0.12f,2.4f),(Color){34,52,60,255});
    bool unlocked=CharUnlocked(viewChar,SV.completed);
    Look lk=CHARS[viewChar].look; if(!unlocked){ lk.skin=(Color){30,36,40,255}; lk.shirt=lk.skin; lk.pants=lk.skin; lk.vest=lk.skin; lk.hat=lk.skin; lk.hair=lk.skin; }
    DrawHuman(&lk,V3(0,0.12f,0),t*0.6f,0,0,0,CHARS[viewChar].w1,true,0,0,CharRank(SV.completed));
    End3D();
    Begin2D(); Title("OPERATORS","Pick any unlocked operator. New operators unlock as you clear missions; every 10 missions all operators rank up.");
    for(int i=0;i<NUM_CHARS;i++){
        Rectangle b=R4(24,120+i*56,270,50); bool ul=CharUnlocked(i,SV.completed);
        DrawRectangleRounded(b,0.2f,8,(i==viewChar)?(Color){40,70,80,255}:(Color){20,32,40,230}); if(i==SV.selChar) DrawRectangleLinesEx(b,2,COL_GREEN);
        DrawText(ul?CHARS[i].call:"???",(int)b.x+14,(int)b.y+6,20,ul?COL_TEXT:COL_DIM);
        DrawText(ul?CHARS[i].role:TextFormat("UNLOCKS AT MISSION %d",CHARS[i].need),(int)b.x+14,(int)b.y+30,12,ul?COL_DIM:COL_GOLD);
        if(uiTapped&&PtIn(uiTapPos,b)){ uiTapped=false; viewChar=i; Sfx(SFX_CLICK,0.7f,1.0f); }
    }
    const CharDef *cd=&CHARS[viewChar]; int rank=CharRank(SV.completed);
    Rectangle ip=R4(uiW-400,120,376,520); Panel(ip,COL_PANEL);
    DrawText(unlocked?cd->call:"LOCKED",(int)ip.x+20,(int)ip.y+16,34,COL_TEXT);
    DrawText(unlocked?cd->name:TextFormat("Complete %d missions to unlock",cd->need),(int)ip.x+20,(int)ip.y+56,16,COL_DIM);
    DrawText(cd->role,(int)ip.x+20,(int)ip.y+82,17,COL_GOLD);
    for(int s=0;s<4;s++) Star(ip.x+30+s*26,ip.y+124,10,s<rank?COL_GOLD:(Color){50,64,72,255});
    DrawText(TextFormat("RANK %d",rank),(int)ip.x+140,(int)ip.y+116,16,COL_TEXT);
    DrawText(cd->desc,(int)ip.x+20,(int)ip.y+150,14,COL_TEXT);
    float rb=1+0.1f*(rank-1), rd=1+0.07f*(rank-1);
    StatBar(ip.x+20,ip.y+190,330,"HEALTH",cd->hpMul*rb/1.9f,COL_GREEN); StatBar(ip.x+20,ip.y+214,330,"SPEED",cd->spdMul/1.3f,(Color){80,170,255,255});
    StatBar(ip.x+20,ip.y+238,330,"DAMAGE",cd->dmgMul*rd/1.5f,COL_RED);
    DrawText(TextFormat("PRIMARY    %s",WEAPONS[cd->w1].name),(int)ip.x+20,(int)ip.y+280,16,COL_TEXT); DrawText(TextFormat("SECONDARY  %s",WEAPONS[cd->w2].name),(int)ip.x+20,(int)ip.y+304,16,COL_TEXT);
    DrawText(TextFormat("GRENADES %d     C4 %d",cd->gren,cd->bombs),(int)ip.x+20,(int)ip.y+332,16,COL_TEXT);
    const char *ab=(cd->regen>0)?"ABILITY: field regeneration":(cd->stealth<0.8f)?"ABILITY: harder to detect":(cd->headMul>1.2f)?"ABILITY: bonus headshot damage":(cd->gren>=5)?"ABILITY: extra explosives":"ABILITY: standard issue";
    DrawText(ab,(int)ip.x+20,(int)ip.y+362,15,COL_GOLD);
    if(unlocked){ if(viewChar==SV.selChar) Btn(R4(ip.x+20,ip.y+430,336,56),"EQUIPPED",1); else if(Btn(R4(ip.x+20,ip.y+430,336,56),"SELECT OPERATOR",0)){ SV.selChar=viewChar; SaveWrite(); } }
    else Btn(R4(ip.x+20,ip.y+430,336,56),"LOCKED",1);
    if(Btn(R4(uiW/2-70,VIRT_H-60,140,44),"BACK",1)) G.screen=(G.prevScreen==SCR_BRIEF)?SCR_BRIEF:SCR_MENU;
    EndMode2D();
}

/* ---------- settings / help / credits ---------- */
static void ScreenSettings(void){
    Background(); Begin2D(); Title("SETTINGS",NULL); Settings *s=&SV.st;
    float lx=uiW/2-430,rx=uiW/2+30,w=400,y0=130,st=74;
    if(Slider(R4(lx,y0,w,40),"MASTER VOLUME",&s->masterVol,0,1,"%.2f")) AudioApplyVolume();
    Slider(R4(lx,y0+st,w,40),"EFFECTS VOLUME",&s->sfxVol,0,1,"%.2f");
    Slider(R4(lx,y0+st*2,w,40),"LOOK SENSITIVITY",&s->sens,0.3f,3.0f,"%.2f");
    Slider(R4(lx,y0+st*3,w,40),"FIELD OF VIEW",&s->fov,60,100,"%.0f");
    static const char *dd[3]={"LOW","MEDIUM","HIGH"}, *df[3]={"EASY","NORMAL","HARD"};
    Cycle(R4(lx,y0+st*4+4,w,48),"DRAW DISTANCE",dd,3,&s->drawDist); Cycle(R4(lx,y0+st*5+4,w,48),"DIFFICULTY",df,3,&s->difficulty);
    Toggle(R4(rx,y0,w,48),"TOUCH CONTROLS",&s->touch); Toggle(R4(rx,y0+st,w,48),"AIM ASSIST (TOUCH)",&s->aimAssist);
    Toggle(R4(rx,y0+st*2,w,48),"INVERT Y AXIS",&s->invertY); Toggle(R4(rx,y0+st*3,w,48),"BLOB SHADOWS",&s->blobShadow);
    Toggle(R4(rx,y0+st*4,w,48),"SHOW FPS",&s->showFps);
#if !defined(PLATFORM_ANDROID)
    if(Btn(R4(rx,y0+st*5,w,48),"TOGGLE FULLSCREEN (F11)",1)) ToggleFullscreen();
#endif
    if(Btn(R4(uiW/2-340,VIRT_H-76,200,50),"DEFAULTS",2)){ Settings d=(Settings){0.9f,0.9f,1.0f,70.0f,false,false,s->touch,true,true,1,1}; *s=d; AudioApplyVolume(); }
    if(Btn(R4(uiW/2-100,VIRT_H-76,200,50),"BACK",0)){ SaveWrite(); G.screen=settingsBack; }
    EndMode2D();
}
static void ScreenHelp(void){
    Background(); Begin2D(); Title("HOW TO PLAY","Destroy the enemy base. Clear every round. Blow up the weapons cache.");
    if(Btn(R4(uiW-170,24,130,40),"BACK",1)) G.screen=SCR_MENU;
    Panel(R4(40,124,(uiW-100)/2,500),COL_PANEL); Panel(R4(60+(uiW-100)/2,124,(uiW-100)/2,500),COL_PANEL);
    float x=64,y=142; const char *k[]={"KEYBOARD + MOUSE","","W A S D        Move","MOUSE           Look / aim","LEFT CLICK      Fire","RIGHT CLICK     Aim down sights","SHIFT           Sprint","SPACE           Jump / handbrake","R               Reload","Q / WHEEL       Swap weapon","G               Throw grenade","B               Plant C4","V               Detonate C4","M               Full map","ESC / P         Pause","F               Enter / exit vehicle"};
    for(int i=0;i<16;i++){ DrawText(k[i],(int)x,(int)(y+i*28),i==0?20:17,i==0?COL_GOLD:COL_TEXT); }
    float x2=84+(uiW-100)/2; const char *t[]={"TOUCH CONTROLS","","Left thumb       Move (push to edge to sprint)","Right side drag   Look around","FIRE / AIM        Hold to shoot / zoom","RLD / SWAP        Reload / change weapon","GREN              Throw grenade","C4 / BOOM         Plant / detonate","Tap the minimap   Open full city map","USE               Enter / exit vehicle","TIPS","Watch DETECTION: ? = suspicious, ! = spotted.","Do NOT shoot citizens: big penalty.","Grab weapons, armor, adrenaline, intel.","Driving: W/S gas+brake, A/D steer."};
    for(int i=0;i<15;i++){ DrawText(t[i],(int)x2,(int)(y+i*28),(i==0||i==10)?20:16,(i==0||i==10)?COL_GOLD:COL_TEXT); }
    EndMode2D();
}
static void ScreenCredits(void){
    Background(); Begin2D(); Title("CREDITS",NULL);
    if(Btn(R4(uiW-170,24,130,40),"BACK",1)) G.screen=SCR_MENU;
    TextC("DHAKA 2099: URBAN WARFARE",uiW/2,170,36,COL_TEXT); TextC("Version " GAME_VERSION,uiW/2,216,18,COL_DIM);
    TextC("Game design, code and procedural art:  your studio",uiW/2,290,20,COL_TEXT);
    TextC("Built with raylib  (raylib.com)  -  zlib/libpng license",uiW/2,330,18,COL_DIM);
    TextC("All sounds and models are generated in code.",uiW/2,366,18,COL_DIM);
    TextC("Set in the streets of Dhaka, Bangladesh.",uiW/2,420,22,COL_GOLD);
    EndMode2D();
}

/* ---------- overlays ---------- */
static void Dim(void){ DrawRectangle(0,0,(int)uiW+1,(int)VIRT_H+1,(Color){0,0,0,170}); }
static void ScreenPause(void){
    DrawGameScene(); Begin2D(); Dim(); TextC("PAUSED",uiW/2,120,56,COL_TEXT);
    float x=uiW/2-170; if(Btn(R4(x,220,340,56),"RESUME",0)) G.screen=SCR_GAME;
    if(Btn(R4(x,290,340,56),M.free?"NEW FREE ROAM":"RESTART MISSION",1)){ G.selMission=M.free?-1:M.idx; deployPending=2; }
    if(Btn(R4(x,360,340,56),"SETTINGS",1)){ settingsBack=SCR_PAUSE; G.screen=SCR_SETTINGS; }
    if(Btn(R4(x,430,340,56),"QUIT TO MENU",2)){ SaveWrite(); G.screen=SCR_MENU; }
    EndMode2D();
}
static void ScreenResult(bool won){
    DrawGameScene(); Begin2D(); Dim(); Rectangle p=R4(uiW/2-300,70,600,580); Panel(p,COL_PANEL);
    TextC(won?"MISSION COMPLETE":"MISSION FAILED",uiW/2,p.y+22,44,won?COL_GREEN:COL_RED);
    TextC(TextFormat("%02d  %s",M.idx+1,M.name),uiW/2,p.y+76,20,COL_DIM);
    if(won) for(int s=0;s<3;s++) Star(uiW/2-60+s*60,p.y+130,22,s<G.rStars?COL_GOLD:(Color){50,64,72,255});
    float y=p.y+180; int sec=(int)G.rTime;
    DrawText(TextFormat("Enemies eliminated     %d",G.rKills),(int)p.x+80,(int)y,20,COL_TEXT); DrawText(TextFormat("Headshots                  %d",G.rHeads),(int)p.x+80,(int)y+30,20,COL_TEXT);
    DrawText(TextFormat("Time                          %02d:%02d",sec/60,sec%60),(int)p.x+80,(int)y+60,20,COL_TEXT); DrawText(TextFormat("Score                        %d",G.rScore),(int)p.x+80,(int)y+90,20,COL_GOLD);
    if(G.rCiv>0) DrawText(TextFormat("Civilians harmed          %d",G.rCiv),(int)p.x+80,(int)y+120,20,COL_RED);
    float by=p.y+470;
    if(won){
        if(G.rNewChar>=0) TextC(TextFormat("NEW OPERATOR UNLOCKED:  %s",CHARS[G.rNewChar].call),uiW/2,y+156,20,COL_GOLD);
        if(G.rRank) TextC(TextFormat("RANK UP!  All operators are now Rank %d",CharRank(SV.completed)),uiW/2,y+184,18,COL_GREEN);
        float bw=176; float bx=uiW/2-bw*1.5f-10;
        if(M.idx+1<MAX_MISSIONS&&Btn(R4(bx,by,bw,56),"NEXT MISSION",0)){ G.selMission=M.idx+1; deployPending=2; }
        if(Btn(R4(bx+bw+10,by,bw,56),"REPLAY",1)){ G.selMission=M.idx; deployPending=2; }
        if(Btn(R4(bx+(bw+10)*2,by,bw,56),"MAIN MENU",1)) G.screen=SCR_MENU;
    } else {
        if(Btn(R4(uiW/2-190,by,180,56),"RETRY",0)){ G.selMission=M.idx; deployPending=2; }
        if(Btn(R4(uiW/2+10,by,180,56),"MAIN MENU",1)) G.screen=SCR_MENU;
    }
    EndMode2D();
}
static void ScreenSplash(void){
    splashT+=GetFrameTime(); Background(); Begin2D(); float a=Clampf(splashT/0.8f,0,1);
    Color w=COL_TEXT; w.a=(unsigned char)(255*a); TextC("DHAKA 2099",uiW/2,VIRT_H/2-70,84,w);
    Color g=COL_GOLD; g.a=(unsigned char)(255*a); TextC("URBAN WARFARE",uiW/2,VIRT_H/2+24,32,g);
    DrawRectangle((int)(uiW/2-120),(int)(VIRT_H/2+70),160,4,COL_GREEN); DrawRectangle((int)(uiW/2+40),(int)(VIRT_H/2+70),80,4,COL_RED);
    TextC("TAP OR PRESS ANY KEY",uiW/2,VIRT_H-60,16,(Color){138,154,166,(unsigned char)(120+100*sinf(splashT*4))});
    EndMode2D();
    if(splashT>3.0f||(splashT>0.8f&&(uiTapped||GetKeyPressed()))){ if(hasProfile) SetupLogin(); else SetupRegister(); }
}
static void ScreenLoading(void){
    Background(); Begin2D(); TextC("LOADING MISSION...",uiW/2,VIRT_H/2-20,34,COL_TEXT); TextC("Generating Dhaka",uiW/2,VIRT_H/2+24,16,COL_DIM); EndMode2D();
}

/* ---------- input for gameplay ---------- */
static void GatherInput(void){
    memset(&IN,0,sizeof(IN));
    if(IsWindowFocused()){
        float sens=0.0022f*SV.st.sens; Vector2 md=GetMouseDelta();
        if(IsCursorHidden()){ IN.look_x+=md.x*sens; IN.look_y+=md.y*sens; }
        IN.move_y=(float)((IsKeyDown(KEY_W)||IsKeyDown(KEY_UP))-(IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN)));
        IN.move_x=(float)((IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT))-(IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT)));
        if(IsCursorHidden()){ IN.fire=IsMouseButtonDown(MOUSE_BUTTON_LEFT); IN.firePressed=IsMouseButtonPressed(MOUSE_BUTTON_LEFT); IN.aim=IsMouseButtonDown(MOUSE_BUTTON_RIGHT); }
        IN.sprint=IsKeyDown(KEY_LEFT_SHIFT); IN.jump=IsKeyPressed(KEY_SPACE); IN.brake=IsKeyDown(KEY_SPACE); IN.use=IsKeyPressed(KEY_F); IN.reload=IsKeyPressed(KEY_R);
        IN.grenade=IsKeyPressed(KEY_G); IN.c4=IsKeyPressed(KEY_B); IN.det=IsKeyPressed(KEY_V);
        IN.swap=IsKeyPressed(KEY_Q)||GetMouseWheelMove()!=0||IsKeyPressed(KEY_ONE)||IsKeyPressed(KEY_TWO);
        IN.pause=IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_P); IN.map=IsKeyPressed(KEY_M);
    }
    if(UiTouchMode()) HudTouchInput();
    if(IN.map) G.mapBig=!G.mapBig;
}

void UiInit(void){
    SetTextureFilter(GetFontDefault().texture,TEXTURE_FILTER_BILINEAR);
    hasProfile=SaveLoad(); if(!hasProfile) SaveDefaults();
    if(!CharUnlocked(SV.selChar,SV.completed)) SV.selChar=0;
    memset(&M,0,sizeof(M)); WorldInit(); WorldGenerate(2,2); GameInit(); viewChar=SV.selChar;
}
void UiFree(void){ UnloadTexture(mapTex); }

void UiFrame(float dt){
    Poll();
    if(IsKeyPressed(KEY_F11)) ToggleFullscreen();
    if(deployPending>0){ deployPending--; if(deployPending==0){ GameStartMission(G.selMission); } return; }
    if(G.screen==SCR_PAUSE&&(IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_P))){ G.screen=SCR_GAME; return; }
    bool wantHidden=(G.screen==SCR_GAME&&!UiTouchMode());
    if(wantHidden&&!IsCursorHidden()) DisableCursor(); if(!wantHidden&&IsCursorHidden()) EnableCursor();
    if(G.screen==SCR_GAME){
        GatherInput();
#if !defined(PLATFORM_ANDROID)&&!defined(PLATFORM_WEB)
        if(!IsWindowFocused()) IN.pause=true;
#endif
        if(IN.pause){ G.screen=SCR_PAUSE; memset(&IN,0,sizeof(IN)); }
        else GameUpdate(dt);
    }
}
void UiDraw(void){
    BeginDrawing(); ClearBackground(COL_BG);
    if(deployPending>0) ScreenLoading();
    else switch(G.screen){
        case SCR_SPLASH: ScreenSplash(); break; case SCR_REGISTER: ScreenRegister(); break; case SCR_LOGIN: ScreenLogin(); break;
        case SCR_MENU: ScreenMenu(); break; case SCR_MISSIONS: ScreenMissions(); break; case SCR_BRIEF: ScreenBrief(); break;
        case SCR_CHARS: ScreenChars(); break; case SCR_SETTINGS: ScreenSettings(); break; case SCR_HELP: ScreenHelp(); break; case SCR_CREDITS: ScreenCredits(); break;
        case SCR_GAME: DrawGameScene(); break; case SCR_PAUSE: ScreenPause(); break;
        case SCR_COMPLETE: ScreenResult(true); break; case SCR_OVER: ScreenResult(false); break;
    }
    EndDrawing();
}
