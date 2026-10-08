/* Characters, weapons and procedural low-poly humans */
#include "game.h"
#define C(r,g,b) {r,g,b,255}

const WeaponDef WEAPONS[NUM_WEAPONS] = {
 /* name,type, dmg, interval, spread, range, reload, recoil, mag, reserve, pellets, auto, sfx, muzzleZ */
 {"GLOCK 17","PISTOL",     22,0.16f,1.2f, 90,1.4f,1.2f, 17,119,1,false,SFX_GLOCK,  0.24f},
 {"MP5","SMG",             17,0.075f,2.2f,80,1.8f,0.8f, 30,210,1,true, SFX_SMG,    0.58f},
 {"AK-47","ASSAULT RIFLE", 30,0.10f,1.6f,170,2.2f,1.6f, 30,180,1,true, SFX_RIFLE,  0.95f},
 {"M4A1","CARBINE",        26,0.085f,1.2f,180,2.0f,1.1f,30,210,1,true, SFX_RIFLE,  0.88f},
 {"REMINGTON 870","SHOTGUN",14,0.90f,5.5f,45,2.4f,3.5f,  6, 36,8,false,SFX_SHOTGUN,1.00f},
 {"SVD DRAGUNOV","SNIPER", 90,1.00f,0.15f,420,2.6f,4.0f, 10, 50,1,false,SFX_SNIPER,1.20f},
 {"PKM","LIGHT MACHINE GUN",28,0.075f,2.6f,140,4.0f,1.3f,100,200,1,true, SFX_RIFLE,  1.05f},
 {"DESERT EAGLE","HAND CANNON",55,0.35f,0.9f,110,1.8f,3.2f,  7, 56,1,false,SFX_DEAGLE, 0.30f},
 {"UZI","MACHINE PISTOL",   14,0.055f,3.4f,60,1.6f,0.7f,  32,256,1,true, SFX_SMG,    0.46f},
 {"FN P90","PDW",           16,0.060f,1.8f,90,2.0f,0.7f,  50,250,1,true, SFX_SMG,    0.55f},
 {"FN SCAR-H","BATTLE RIFLE",38,0.110f,1.2f,200,2.3f,2.0f, 20,140,1,true, SFX_RIFLE,  0.98f},
 {"SPAS-12","COMBAT SHOTGUN",12,0.50f,5.0f,45,2.6f,3.0f,   8, 40,9,false,SFX_SHOTGUN,1.00f},
 {"AWM","BOLT SNIPER",     150,1.50f,0.05f,600,2.8f,5.0f,  5, 30,1,false,SFX_SNIPER, 1.35f},
 {"M79","GRENADE LAUNCHER",170,1.20f,0.4f,120,2.2f,3.0f,   1,  9,1,false,SFX_LAUNCH, 0.95f,1},
};

const CharDef CHARS[NUM_CHARS] = {
 {"RAFIQ AHMED","GHOST","RECON SCOUT","Balanced all-rounder. Reliable under any fire.",0,
   1.00f,1.00f,1.00f,1.0f,0,1.0f, W_AK47,W_GLOCK,3,1,
   {C(150,104,72),C(70,85,60),C(60,70,52),C(45,52,40),C(60,70,52),C(20,16,14),1,1,false,false}},
 {"NUSRAT JAHAN","VIXEN","CQB SPECIALIST","Fast and deadly up close. Extra sprint speed.",2,
   0.95f,1.14f,1.00f,1.0f,0,1.0f, W_P90,W_DEAGLE,3,1,
   {C(176,125,92),C(40,60,90),C(35,45,60),C(30,36,48),C(0,0,0),C(25,18,16),0,2,false,false}},
 {"TARIQ HASAN","BULLDOZER","HEAVY GUNNER","Massive health pool and a belt-fed PKM.",4,
   1.45f,0.88f,1.00f,1.0f,0,1.0f, W_PKM,W_SPAS,3,1,
   {C(120,80,55),C(80,60,45),C(55,50,42),C(95,80,55),C(70,75,60),C(20,16,14),2,0,true,false}},
 {"ANIKA RAHMAN","HAWK","MARKSMAN","Deadly headshots with the SVD Dragunov.",7,
   0.90f,1.00f,1.00f,1.7f,0,1.0f, W_SVD,W_GLOCK,3,1,
   {C(186,136,100),C(60,66,70),C(48,52,56),C(40,44,48),C(50,55,50),C(24,18,16),3,2,false,false}},
 {"IMRAN KHAN","SPARKS","DEMOLITIONS","Carries double grenades and extra C4.",10,
   1.05f,0.98f,1.00f,1.0f,0,1.0f, W_M79,W_DEAGLE,7,3,
   {C(140,98,68),C(150,80,35),C(60,56,48),C(110,70,35),C(210,170,40),C(20,16,14),2,0,false,true}},
 {"SADIA ISLAM","ANGEL","FIELD MEDIC","Regenerates health during combat.",13,
   1.00f,1.02f,1.00f,1.0f,1.6f,1.0f, W_M4A1,W_GLOCK,3,1,
   {C(170,120,88),C(210,210,205),C(60,90,70),C(190,60,60),C(220,220,215),C(26,18,16),6,3,false,false}},
 {"FARHAN CHOWDHURY","SHADOW","STEALTH OPERATIVE","Enemies detect him much later.",16,
   0.95f,1.06f,1.05f,1.0f,0,0.45f, W_MP5,W_GLOCK,3,2,
   {C(130,90,62),C(28,30,36),C(26,28,34),C(20,22,28),C(24,26,30),C(20,16,14),5,0,false,false}},
 {"MITU AKTER","BLAZE","ASSAULT TROOPER","High damage output with the M4A1.",20,
   1.05f,1.04f,1.22f,1.0f,0,1.0f, W_SCAR,W_UZI,4,1,
   {C(165,115,82),C(130,40,40),C(50,40,40),C(70,30,30),C(0,0,0),C(20,15,12),0,2,false,true}},
 {"ZAHID MAHMUD","IRON","VETERAN COMMANDO","Armored veteran. Tough and hard-hitting.",24,
   1.30f,0.97f,1.12f,1.1f,0,0.9f, W_AK47,W_SHOTGUN,4,2,
   {C(115,76,52),C(95,95,100),C(72,74,80),C(62,66,72),C(80,84,90),C(30,26,24),2,0,true,true}},
 {"CMDR SHIRIN KARIM","PHOENIX","ELITE COMMANDER","The best of everything. Leads from the front.",28,
   1.25f,1.10f,1.20f,1.3f,0.8f,0.7f, W_SCAR,W_AWM,5,2,
   {C(180,130,96),C(30,30,34),C(30,30,34),C(150,120,50),C(140,30,40),C(20,14,12),6,3,false,false}},
};

int CharRank(int completed){ int r=1+completed/10; return r>4?4:r; }
bool CharUnlocked(int i,int completed){ return completed>=CHARS[i].need; }

/* ---------- shaded box (fake directional light, no shaders needed) ---------- */
void BoxC(Vector3 c,Vector3 s,Color col){
    float x0=c.x-s.x*0.5f,x1=c.x+s.x*0.5f,y0=c.y-s.y*0.5f,y1=c.y+s.y*0.5f,z0=c.z-s.z*0.5f,z1=c.z+s.z*0.5f;
    rlCheckRenderBatchLimit(24);
    rlSetTexture(rlGetTextureIdDefault());
    rlBegin(RL_QUADS);
#define SH(f) rlColor4ub((unsigned char)(col.r*(f)),(unsigned char)(col.g*(f)),(unsigned char)(col.b*(f)),col.a)
    SH(1.00f); rlVertex3f(x0,y1,z1);rlVertex3f(x1,y1,z1);rlVertex3f(x1,y1,z0);rlVertex3f(x0,y1,z0);
    SH(0.82f); rlVertex3f(x0,y0,z1);rlVertex3f(x1,y0,z1);rlVertex3f(x1,y1,z1);rlVertex3f(x0,y1,z1);
    SH(0.55f); rlVertex3f(x1,y0,z0);rlVertex3f(x0,y0,z0);rlVertex3f(x0,y1,z0);rlVertex3f(x1,y1,z0);
    SH(0.70f); rlVertex3f(x1,y0,z1);rlVertex3f(x1,y0,z0);rlVertex3f(x1,y1,z0);rlVertex3f(x1,y1,z1);
    SH(0.62f); rlVertex3f(x0,y0,z0);rlVertex3f(x0,y0,z1);rlVertex3f(x0,y1,z1);rlVertex3f(x0,y1,z0);
    SH(0.40f); rlVertex3f(x0,y0,z0);rlVertex3f(x1,y0,z0);rlVertex3f(x1,y0,z1);rlVertex3f(x0,y0,z1);
#undef SH
    rlEnd();
}
void DrawFlat(float cx,float y,float cz,float w,float d,Color c){
    rlCheckRenderBatchLimit(4);
    rlSetTexture(rlGetTextureIdDefault());
    rlBegin(RL_QUADS); rlColor4ub(c.r,c.g,c.b,c.a);
    rlVertex3f(cx-w*0.5f,y,cz+d*0.5f);rlVertex3f(cx+w*0.5f,y,cz+d*0.5f);
    rlVertex3f(cx+w*0.5f,y,cz-d*0.5f);rlVertex3f(cx-w*0.5f,y,cz-d*0.5f);
    rlEnd();
}

/* ---------- weapon models (forward = +Z, origin at the grip) ---------- */
typedef struct { float x,y,z,w,h,l; unsigned char c; } WP;
static const Color WPC[5]={{28,28,32,255},{58,60,66,255},{120,78,40,255},{150,130,95,255},{140,145,152,255}};
static const WP W_GL[]={{0,0.045f,0.10f,0.04f,0.055f,0.22f,0},{0,-0.04f,0.0f,0.035f,0.11f,0.05f,1},{0,0.0f,0.21f,0.02f,0.02f,0.04f,4},{0}};
static const WP W_MP[]={{0,0.03f,0.22f,0.05f,0.08f,0.40f,0},{0,-0.10f,0.20f,0.03f,0.16f,0.05f,1},{0,0.04f,0.52f,0.025f,0.025f,0.20f,4},{0,0.03f,-0.12f,0.04f,0.06f,0.22f,0},{0,-0.06f,0.35f,0.04f,0.08f,0.06f,1},{0}};
static const WP W_AK[]={{0,0.03f,0.25f,0.05f,0.08f,0.50f,1},{0,0.02f,0.58f,0.056f,0.07f,0.28f,2},{0,0.05f,0.88f,0.02f,0.02f,0.34f,4},{0,-0.12f,0.30f,0.04f,0.20f,0.07f,0},{0,0.0f,-0.14f,0.045f,0.10f,0.32f,2},{0,0.09f,1.04f,0.012f,0.04f,0.012f,0},{0}};
static const WP W_M4[]={{0,0.03f,0.22f,0.05f,0.09f,0.52f,0},{0,0.10f,0.18f,0.03f,0.04f,0.30f,1},{0,0.04f,0.62f,0.05f,0.06f,0.26f,1},{0,0.045f,0.88f,0.02f,0.02f,0.28f,4},{0,-0.11f,0.27f,0.035f,0.18f,0.07f,0},{0,0.0f,-0.14f,0.045f,0.09f,0.30f,0},{0}};
static const WP W_SG[]={{0,0.04f,0.60f,0.035f,0.035f,0.90f,4},{0,0.0f,0.20f,0.05f,0.08f,0.34f,1},{0,-0.01f,0.52f,0.055f,0.05f,0.22f,2},{0,0.0f,-0.14f,0.045f,0.10f,0.34f,2},{0}};
static const WP W_SV[]={{0,0.03f,0.30f,0.045f,0.07f,0.60f,2},{0,0.045f,0.95f,0.02f,0.02f,0.50f,0},{0,0.11f,0.32f,0.04f,0.04f,0.26f,0},{0,0.0f,-0.14f,0.045f,0.11f,0.36f,2},{0,-0.08f,0.28f,0.035f,0.12f,0.06f,1},{0}};
static const WP W_PK[]={{0,0.03f,0.30f,0.06f,0.09f,0.70f,1},{0,-0.14f,0.30f,0.17f,0.15f,0.10f,3},{0,0.05f,1.00f,0.025f,0.025f,0.40f,4},{0,0.0f,-0.14f,0.045f,0.10f,0.30f,2},{0,-0.06f,0.95f,0.1f,0.1f,0.02f,1},{0}};
static const WP W_DE[]={{0,0.05f,0.12f,0.045f,0.07f,0.30f,4},{0,-0.05f,0.0f,0.04f,0.12f,0.055f,1},{0,0.0f,0.28f,0.03f,0.04f,0.06f,0},{0}};
static const WP W_UZ[]={{0,0.03f,0.14f,0.05f,0.09f,0.30f,0},{0,-0.12f,0.10f,0.03f,0.20f,0.04f,1},{0,0.03f,-0.06f,0.03f,0.04f,0.15f,1},{0,0.04f,0.34f,0.02f,0.02f,0.10f,4},{0}};
static const WP W_P9[]={{0,0.03f,0.12f,0.06f,0.10f,0.50f,0},{0,0.085f,0.15f,0.04f,0.03f,0.28f,1},{0,-0.07f,0.05f,0.04f,0.12f,0.05f,1},{0,0.03f,0.44f,0.04f,0.05f,0.12f,1},{0}};
static const WP W_SC[]={{0,0.03f,0.25f,0.055f,0.09f,0.55f,3},{0,0.035f,0.62f,0.05f,0.07f,0.30f,3},{0,0.045f,0.90f,0.02f,0.02f,0.30f,4},{0,-0.12f,0.27f,0.04f,0.18f,0.07f,0},{0,0.0f,-0.14f,0.05f,0.10f,0.32f,3},{0,0.10f,0.25f,0.03f,0.04f,0.20f,0},{0}};
static const WP W_SP[]={{0,0.04f,0.60f,0.04f,0.04f,0.80f,0},{0,-0.005f,0.60f,0.035f,0.035f,0.70f,1},{0,0.0f,0.20f,0.05f,0.08f,0.34f,0},{0,0.0f,-0.14f,0.04f,0.10f,0.34f,0},{0,-0.03f,0.50f,0.06f,0.05f,0.18f,1},{0}};
static const WP W_AW[]={{0,0.03f,0.30f,0.05f,0.07f,0.55f,0},{0,0.045f,1.00f,0.025f,0.025f,0.70f,1},{0,0.11f,0.35f,0.045f,0.045f,0.32f,0},{0,0.0f,-0.14f,0.05f,0.12f,0.40f,1},{0,-0.08f,0.30f,0.035f,0.12f,0.06f,1},{0}};
static const WP W_M7[]={{0,0.04f,0.45f,0.10f,0.10f,0.90f,1},{0,0.0f,-0.10f,0.04f,0.10f,0.30f,2},{0,-0.06f,0.20f,0.04f,0.12f,0.05f,2},{0,0.12f,0.25f,0.02f,0.05f,0.02f,4},{0}};
static const WP *WMODEL[NUM_WEAPONS]={W_GL,W_MP,W_AK,W_M4,W_SG,W_SV,W_PK,W_DE,W_UZ,W_P9,W_SC,W_SP,W_AW,W_M7};
static const float WGRIP[NUM_WEAPONS]={0.06f,0.36f,0.52f,0.48f,0.50f,0.56f,0.56f,0.07f,0.26f,0.34f,0.50f,0.50f,0.62f,0.50f};
#define HOLD_X (-0.10f)
#define HOLD_Y (-0.14f)
#define HOLD_Z (0.28f)
#define SHOULDER_Y 1.42f

static void DrawWeapon(int w){
    for(const WP *p=WMODEL[w]; p->w>0; p++) BoxC(V3(p->x,p->y,p->z),V3(p->w,p->h,p->l),WPC[p->c]);
}
void DrawWeaponWorld(int w,Vector3 pos,float yaw,float scale){
    rlPushMatrix(); rlTranslatef(pos.x,pos.y,pos.z); rlRotatef(yaw*RAD2DEG,0,1,0); rlScalef(scale,scale,scale);
    rlTranslatef(0,0,-WEAPONS[w].muzzleZ*0.4f); DrawWeapon(w); rlPopMatrix();
}
Vector3 MuzzlePos(Vector3 pos,float yaw,float pitch,int weapon,bool ready){
    float up = ready ? pitch : pitch-0.5f;
    float lx=HOLD_X, ly=HOLD_Y+0.04f, lz=HOLD_Z+WEAPONS[weapon].muzzleZ;
    float cy=cosf(up), sy=sinf(up);
    float y2=ly*cy+lz*sy, z2=-ly*sy+lz*cy; y2+=SHOULDER_Y;
    float cs=cosf(yaw), ss=sinf(yaw);
    return V3(pos.x+lx*cs+z2*ss, pos.y+y2, pos.z-lx*ss+z2*cs);
}
/* box stretched between two points (long axis = Z before rotation) */
static void Limb(Vector3 a,Vector3 b,float th,Color col){
    Vector3 d=V3Sub(b,a); float len=V3Len(d); if(len<1e-4f) return;
    float yaw=atan2f(d.x,d.z), pit=-asinf(Clampf(d.y/len,-1,1));
    rlPushMatrix(); rlTranslatef((a.x+b.x)*0.5f,(a.y+b.y)*0.5f,(a.z+b.z)*0.5f);
    rlRotatef(yaw*RAD2DEG,0,1,0); rlRotatef(pit*RAD2DEG,1,0,0);
    BoxC(V3(0,0,0),V3(th,th,len),col); rlPopMatrix();
}

void DrawHuman(const Look *lk,Vector3 pos,float yaw,float walk,float speedN,float pitch,
               int weapon,bool ready,float fall,float flash,int rank){
    Color skin=lk->skin,shirt=lk->shirt,pants=lk->pants,vest=lk->vest,hat=lk->hat,hair=lk->hair;
    if(flash>0){ skin=ColMix(skin,WHITE,flash); shirt=ColMix(shirt,WHITE,flash); pants=ColMix(pants,WHITE,flash); vest=ColMix(vest,WHITE,flash); }
    Color boot={30,26,24,255}, black={18,18,20,255};
    rlPushMatrix();
    rlTranslatef(pos.x,pos.y,pos.z);
    rlRotatef(yaw*RAD2DEG,0,1,0);
    if(fall>0){ rlTranslatef(0,0.17f*fall,0); rlRotatef(-90*fall,1,0,0); }
    float sw=sinf(walk)*speedN*0.8f;
    for(int s=-1;s<=1;s+=2){                                   /* legs */
        rlPushMatrix(); rlTranslatef(0.12f*s,0.92f,0); rlRotatef(sw*s*RAD2DEG,1,0,0);
        BoxC(V3(0,-0.42f,0),V3(0.20f,0.84f,0.24f),pants);
        BoxC(V3(0,-0.84f,0.04f),V3(0.21f,0.18f,0.32f),boot);
        rlPopMatrix();
    }
    if(lk->skirt) BoxC(V3(0,0.52f,0),V3(0.45f,0.78f,0.31f),lk->skirtCol);
    BoxC(V3(0,1.22f,0),V3(0.46f,0.56f,0.26f),shirt);           /* torso + gear */
    if(weapon>=0){
        BoxC(V3(0,1.25f,0.01f),V3(0.50f,0.42f,0.30f),vest);
        BoxC(V3(0,1.07f,0.17f),V3(0.36f,0.11f,0.08f),ColMul(vest,0.75f));
        BoxC(V3(0,0.97f,0),V3(0.48f,0.06f,0.28f),black);
        BoxC(V3(0,1.25f,-0.19f),V3(0.34f,0.36f,0.10f),ColMul(vest,0.85f));
    } else BoxC(V3(0,0.98f,0),V3(0.47f,0.06f,0.27f),ColMul(lk->pants,0.8f));
    for(int i=0;i<rank-1;i++) BoxC(V3(0.13f,1.37f-i*0.05f,0.165f),V3(0.1f,0.025f,0.01f),(Color){230,190,60,255});
    if(rank>=3){ BoxC(V3(-0.29f,1.47f,0),V3(0.12f,0.04f,0.16f),(Color){230,190,60,255}); BoxC(V3(0.29f,1.47f,0),V3(0.12f,0.04f,0.16f),(Color){230,190,60,255}); }
    BoxC(V3(0,1.53f,0),V3(0.1f,0.1f,0.1f),skin);               /* neck + head */
    BoxC(V3(0,1.67f,0),V3(0.22f,0.24f,0.23f),skin);
    if(lk->hatType!=5&&lk->hatType!=7){
        if(!lk->shades){ BoxC(V3(-0.05f,1.69f,0.118f),V3(0.035f,0.03f,0.01f),black); BoxC(V3(0.05f,1.69f,0.118f),V3(0.035f,0.03f,0.01f),black); }
        else BoxC(V3(0,1.69f,0.12f),V3(0.2f,0.05f,0.02f),black);
        BoxC(V3(0,1.61f,0.118f),V3(0.05f,0.015f,0.01f),ColMul(skin,0.6f));
    }
    if(lk->beard) BoxC(V3(0,1.60f,0.10f),V3(0.2f,0.1f,0.07f),ColMul(hair,1.0f));
    if(lk->hairType==1) BoxC(V3(0,1.79f,-0.01f),V3(0.24f,0.07f,0.25f),hair);
    if(lk->hairType==2){ BoxC(V3(0,1.79f,-0.01f),V3(0.24f,0.07f,0.25f),hair); BoxC(V3(0,1.62f,-0.17f),V3(0.09f,0.32f,0.08f),hair); }
    if(lk->hairType==3){ BoxC(V3(0,1.79f,-0.01f),V3(0.24f,0.07f,0.25f),hair); BoxC(V3(0,1.86f,-0.08f),V3(0.12f,0.1f,0.12f),hair); }
    switch(lk->hatType){
        case 1: BoxC(V3(0,1.80f,0),V3(0.25f,0.07f,0.26f),hat); BoxC(V3(0,1.77f,0.17f),V3(0.24f,0.02f,0.12f),hat); break;
        case 2: BoxC(V3(0,1.80f,0),V3(0.27f,0.15f,0.28f),hat); BoxC(V3(0,1.73f,0.0f),V3(0.285f,0.02f,0.29f),ColMul(hat,0.8f)); break;
        case 3: BoxC(V3(0,1.80f,0),V3(0.25f,0.12f,0.26f),hat); break;
        case 4: BoxC(V3(0,1.62f,0.116f),V3(0.23f,0.1f,0.02f),hat); break;
        case 5: BoxC(V3(0,1.67f,0),V3(0.245f,0.265f,0.255f),hat); BoxC(V3(0,1.69f,0.13f),V3(0.17f,0.045f,0.01f),skin); break;
        case 6: BoxC(V3(0.03f,1.80f,0),V3(0.27f,0.05f,0.27f),hat); break;
        case 7: BoxC(V3(0,1.70f,-0.015f),V3(0.27f,0.30f,0.27f),hat); BoxC(V3(0,1.46f,-0.01f),V3(0.52f,0.20f,0.32f),hat);
                BoxC(V3(0,1.66f,0.14f),V3(0.17f,0.17f,0.012f),skin);
                BoxC(V3(-0.04f,1.69f,0.148f),V3(0.03f,0.025f,0.006f),black); BoxC(V3(0.04f,1.69f,0.148f),V3(0.03f,0.025f,0.006f),black); break;
        case 8: BoxC(V3(0,1.79f,0),V3(0.235f,0.055f,0.245f),(Color){240,240,236,255}); break;
        default: break;
    }
    if(weapon<0){
        for(int s=-1;s<=1;s+=2){
            float sz=-sinf(walk)*speedN*0.30f*s;
            Vector3 sh=V3(0.28f*s,1.42f,0), el=V3(0.30f*s,1.15f,sz*0.5f), hd=V3(0.30f*s,0.90f,sz);
            Limb(sh,el,0.11f,shirt); Limb(el,hd,0.095f,skin); BoxC(hd,V3(0.08f,0.08f,0.09f),skin);
        }
    } else {
    /* upper body group: arms + weapon, rotates with aim pitch */
    rlPushMatrix(); rlTranslatef(0,SHOULDER_Y,0);
    float up = ready ? pitch : pitch-0.5f;
    rlRotatef(-up*RAD2DEG,1,0,0);
    Vector3 gripR=V3(HOLD_X,HOLD_Y,HOLD_Z+0.02f), gripL=V3(HOLD_X+0.07f,HOLD_Y-0.01f,HOLD_Z+WGRIP[weapon]);
    if(weapon==W_GLOCK) gripL=V3(HOLD_X+0.03f,HOLD_Y-0.02f,HOLD_Z+0.05f);
    Vector3 shR=V3(-0.28f,0,0), shL=V3(0.28f,0,0);
    Vector3 elR=V3(-0.30f,-0.28f,0.14f), elL=V3(0.26f,-0.24f,0.2f);
    Limb(shR,elR,0.11f,shirt); Limb(elR,gripR,0.095f,shirt); BoxC(gripR,V3(0.08f,0.08f,0.1f),skin);
    Limb(shL,elL,0.11f,shirt); Limb(elL,gripL,0.095f,shirt); BoxC(gripL,V3(0.08f,0.08f,0.1f),skin);
    rlPushMatrix(); rlTranslatef(HOLD_X,HOLD_Y,HOLD_Z); DrawWeapon(weapon); rlPopMatrix();
    rlPopMatrix();
    }
    rlPopMatrix();
}

/* cheap model for distant crowds: ~7 boxes */
void DrawHumanLow(const Look *lk,Vector3 pos,float yaw,float walk,float speedN,float fall){
    rlPushMatrix(); rlTranslatef(pos.x,pos.y,pos.z); rlRotatef(yaw*RAD2DEG,0,1,0);
    if(fall>0){ rlTranslatef(0,0.17f*fall,0); rlRotatef(-90*fall,1,0,0); }
    float sw=sinf(walk)*speedN*0.8f;
    for(int s=-1;s<=1;s+=2){ rlPushMatrix(); rlTranslatef(0.12f*s,0.92f,0); rlRotatef(sw*s*RAD2DEG,1,0,0); BoxC(V3(0,-0.45f,0),V3(0.2f,0.9f,0.24f),lk->pants); rlPopMatrix(); }
    if(lk->skirt) BoxC(V3(0,0.52f,0),V3(0.45f,0.78f,0.31f),lk->skirtCol);
    BoxC(V3(0,1.22f,0),V3(0.5f,0.58f,0.28f),lk->shirt);
    BoxC(V3(0,1.67f,0),V3(0.22f,0.24f,0.23f),lk->skin);
    if(lk->hatType==7) BoxC(V3(0,1.62f,-0.01f),V3(0.27f,0.40f,0.27f),lk->hat);
    else if(lk->hatType) BoxC(V3(0,1.80f,0),V3(0.25f,0.09f,0.26f),lk->hatType==8?(Color){240,240,236,255}:lk->hat);
    else if(lk->hairType) BoxC(V3(0,1.79f,-0.01f),V3(0.24f,0.07f,0.25f),lk->hair);
    rlPopMatrix();
}
