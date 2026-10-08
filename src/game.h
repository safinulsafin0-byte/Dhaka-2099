/* DHAKA 2099: URBAN WARFARE - shared types and prototypes */
#ifndef GAME_H
#define GAME_H
#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define GAME_TITLE   "DHAKA 2099: URBAN WARFARE"
#define GAME_VERSION "1.0.0"
#define SAVE_FILE    "data/save.dat"
#define VIRT_H       720.0f

#define BLOCKS       12
#define BLOCK_SZ     200.0f
#define ROAD_W       30.0f
#define WORLD_SZ     (BLOCKS*BLOCK_SZ)
#define MAX_OBS      6000
#define MAX_DECO     12000
#define MAX_ENEMY    40
#define MAX_PART     800
#define MAX_TRACER   64
#define MAX_PICKUP   260
#define MAX_CIT      130
#define MAX_VEH      56
#define MAX_STALL    600
#define MAX_GREN     32
#define MAX_C4       6
#define MAX_BARREL   48
#define MAX_MISSIONS 30
#define NUM_CHARS    10
#define NUM_WEAPONS  14
#define MAP_MARGIN   100.0f
#define MAP_PX       2560

/* ---------- small math helpers ---------- */
static inline Vector3 V3(float x,float y,float z){ return (Vector3){x,y,z}; }
static inline Vector3 V3Add(Vector3 a,Vector3 b){ return (Vector3){a.x+b.x,a.y+b.y,a.z+b.z}; }
static inline Vector3 V3Sub(Vector3 a,Vector3 b){ return (Vector3){a.x-b.x,a.y-b.y,a.z-b.z}; }
static inline Vector3 V3Mul(Vector3 a,float s){ return (Vector3){a.x*s,a.y*s,a.z*s}; }
static inline float   V3Dot(Vector3 a,Vector3 b){ return a.x*b.x+a.y*b.y+a.z*b.z; }
static inline float   V3Len(Vector3 a){ return sqrtf(V3Dot(a,a)); }
static inline Vector3 V3Norm(Vector3 a){ float l=V3Len(a); return l>1e-6f?V3Mul(a,1.0f/l):(Vector3){0,0,0}; }
static inline float   V3Dist(Vector3 a,Vector3 b){ return V3Len(V3Sub(a,b)); }
static inline float   V3DistXZ(Vector3 a,Vector3 b){ float dx=a.x-b.x,dz=a.z-b.z; return sqrtf(dx*dx+dz*dz); }
static inline float   Clampf(float v,float a,float b){ return v<a?a:(v>b?b:v); }
static inline float   Lerpf(float a,float b,float t){ return a+(b-a)*t; }
static inline float   Rnd01(void){ return (float)GetRandomValue(0,10000)/10000.0f; }
static inline float   RndR(float a,float b){ return a+(b-a)*Rnd01(); }
static inline Vector3 YawDir(float yaw){ return (Vector3){sinf(yaw),0,cosf(yaw)}; }
static inline Vector3 YawRight(float yaw){ return (Vector3){-cosf(yaw),0,sinf(yaw)}; }
static inline float   AngleDiff(float a,float b){ float d=fmodf(b-a+PI,2*PI); if(d<0)d+=2*PI; return d-PI; }
static inline Color   ColMul(Color c,float f){ return (Color){(unsigned char)Clampf(c.r*f,0,255),(unsigned char)Clampf(c.g*f,0,255),(unsigned char)Clampf(c.b*f,0,255),c.a}; }
static inline Color   ColMix(Color a,Color b,float t){ t=Clampf(t,0,1); return (Color){(unsigned char)(a.r+(b.r-a.r)*t),(unsigned char)(a.g+(b.g-a.g)*t),(unsigned char)(a.b+(b.b-a.b)*t),(unsigned char)(a.a+(b.a-a.a)*t)}; }

/* ---------- settings / save ---------- */
typedef struct {
    float masterVol, sfxVol, sens, fov;
    bool invertY, showFps, touch, blobShadow, aimAssist;
    int drawDist;      /* 0 low, 1 medium, 2 high */
    int difficulty;    /* 0 easy, 1 normal, 2 hard */
} Settings;

typedef struct {
    unsigned magic; int version; bool exists;
    char name[40], username[24], hint[64];
    unsigned long long salt, passHash;
    int completed;                       /* missions completed (0..MAX_MISSIONS) */
    unsigned char stars[MAX_MISSIONS];
    int selChar;
    int totalKills, totalHeadshots, totalScore;
    float playTime;
    Settings st;
} Save;
extern Save SV;
void SaveDefaults(void);
bool SaveLoad(void);
void SaveWrite(void);
void SaveRegister(const char *name,const char *user,const char *pass,const char *hint);
bool SaveCheckLogin(const char *user,const char *pass);

/* ---------- characters & weapons ---------- */
typedef struct {
    Color skin, shirt, pants, vest, hat, hair;
    unsigned char hatType;   /* 0 none 1 cap 2 helmet 3 beanie 4 mask 5 balaclava 6 beret */
    unsigned char hairType;  /* 0 bald 1 short 2 ponytail 3 bun */
    bool beard, shades;
    bool skirt; Color skirtCol;   /* lungi / saree / long dress */
} Look;
typedef struct {
    const char *name,*call,*role,*desc;
    int need;                 /* missions completed to unlock */
    float hpMul,spdMul,dmgMul,headMul,regen,stealth;
    int w1,w2,gren,bombs;
    Look look;
} CharDef;
enum { W_GLOCK,W_MP5,W_AK47,W_M4A1,W_SHOTGUN,W_SVD,W_PKM,W_DEAGLE,W_UZI,W_P90,W_SCAR,W_SPAS,W_AWM,W_M79 };
typedef struct {
    const char *name,*type;
    float dmg,interval,spread,range,reload,recoil;
    int mag,reserve,pellets; bool automatic; int sfx; float muzzleZ;
    int kind;                 /* 0 bullet, 1 grenade launcher */
} WeaponDef;
extern const CharDef CHARS[NUM_CHARS];
extern const WeaponDef WEAPONS[NUM_WEAPONS];
int  CharRank(int completed);
bool CharUnlocked(int i,int completed);
void BoxC(Vector3 c,Vector3 s,Color col);          /* shaded box, current rlgl matrix */
void DrawHuman(const Look *lk,Vector3 pos,float yaw,float walk,float speedN,float pitch,
               int weapon,bool ready,float fall,float flash,int rank);   /* weapon<0 = unarmed */
void DrawHumanLow(const Look *lk,Vector3 pos,float yaw,float walk,float speedN,float fall);
Vector3 MuzzlePos(Vector3 pos,float yaw,float pitch,int weapon,bool ready);
void DrawFlat(float cx,float y,float cz,float w,float d,Color c);
void DrawWeaponWorld(int w,Vector3 pos,float yaw,float scale);

/* ---------- audio ---------- */
enum { SFX_GLOCK,SFX_SMG,SFX_RIFLE,SFX_SHOTGUN,SFX_SNIPER,SFX_EXPLODE,SFX_RELOAD,SFX_PICK,
       SFX_HIT,SFX_HURT,SFX_CLICK,SFX_BEEP,SFX_PIN,SFX_EMPTY,SFX_HEAD,SFX_DEAGLE,SFX_LAUNCH,SFX_ALERT,SFX_SIREN,SFX_RADIO,SFX_COUNT };
void AudioInitAll(void);
void AudioFreeAll(void);
void AudioApplyVolume(void);
void Sfx(int id,float vol,float pitch);
void Sfx3D(int id,Vector3 at,float pitch);
void SirenUpdate(float vol,float dt);             /* looping police siren, vol 0 stops it */
extern Vector3 gListenPos; extern float gListenYaw;

/* ---------- world ---------- */
enum { OB_BUILD,OB_SOLID,OB_VEH,OB_WATER,OB_BARREL,OB_CACHE,OB_WALL };
typedef struct { Vector3 mn,mx; unsigned char kind,style; bool active; Color col; } Obs;
enum { DK_TREE,DK_LAMP,DK_BOAT,DK_DOME,DK_FLAG,DK_TENT,DK_FLOOD,DK_QUAD,DK_MINAR,DK_STACK,DK_AWNING,DK_BILL,DK_POLE,DK_SIGNAL };
typedef struct { unsigned char kind; Vector3 p; float a,b; Color col; } Deco;
typedef struct { Vector3 pos; float hp,fuse; bool alive; int obs; } Barrel;
typedef struct { int br,bc; Vector3 center,cache; float x0,z0; } BaseInfo;
extern Obs OBS[MAX_OBS]; extern int nObs;
extern Deco DECO[MAX_DECO]; extern int nDeco;
extern Barrel BARREL[MAX_BARREL]; extern int nBarrel;
extern BaseInfo BASE;
typedef struct { Vector3 pos; float yaw; bool taken; } Stall;
extern Stall STALLS[MAX_STALL]; extern int nStall;
extern Texture2D mapTex;
void  WorldInit(void);
void  WorldGenerate(int baseR,int baseC);
void  WorldDraw(Vector3 cam,Vector3 camFwd,int distLevel,float t);
float WorldRayT(Vector3 o,Vector3 d,float maxT,int *obsIdx);
bool  RayAABB(Vector3 o,Vector3 d,Vector3 mn,Vector3 mx,float *t);
void  MoveCollide(Vector3 *p,float r,float *groundOut);
bool  WorldFree(float x,float z,float r);
void  NavBuild(void);
void  NavFlow(Vector3 target);
bool  NavDir(Vector3 from,Vector3 *dirOut);
const char *DistrictAt(float x,float z);
bool  BlockValidForBase(int r,int c);
void  Begin3D(Camera3D cam);
void  End3D(void);
bool  IsWaterAt(float x,float z);

/* ---------- gameplay ---------- */
enum { EN_RIFLE,EN_SMG,EN_SHOTGUN,EN_SNIPER,EN_HEAVY,EN_COMMANDER };
typedef struct {
    Vector3 pos; float vy,yaw,pitch,hp,maxHp; bool alive,onGround;
    int cw,wid[2],mag[2],res[2]; float fireT,reloadT; bool reloading;
    int gren,bombs; float walk,speedN; bool aiming,sprinting;
    float recoil,muzzleT,regenAcc,stepT;
    int kills,heads,score; float dmgTaken; int rank;
    bool inVeh; int vehIdx; int civKills;
    float armor,adren;
} Player;
typedef struct {
    bool active,dead,isStatic; int type,wid,state; Vector3 pos,vel,home,dest,lastSeen;
    float alertT,yaw,hp,maxHp,alert,fireT,strafeT,strafeDir,walk,speedN,deadT,fall,flash,spot,grenT,stuck,los,sideT,lostT,speed,prefMin,prefMax;
    int burst; bool canSee; Look look;
    bool cop;                 /* police officer (crime system) - not counted as a mission hostile */
} Enemy;
typedef struct { bool active,dead,vendor; Vector3 pos,threat; float yaw,hp,walk,speedN,fall,deadT,flee,stuck,speed,turnCd,line,lastT; int axis,dir,stall; Look look; } Citizen;
typedef struct { bool active,player,parked,wreck,braking; int type,axis,dir,lineIdx; Vector3 pos; float yaw,speed,hp,maxHp,life,turnCd; Color col;
    bool police,deployed; float pt,stuck; } Vehicle;
typedef struct { Vector3 pos; int type; bool active; float t; int wid; float delay; } Pickup;   /* 0 hp 1 ammo 2 gren 3 bomb 4 weapon 5 armor 6 adrenaline 7 intel cash 8 first-aid kit (full heal) */
typedef struct { Vector3 pos,vel; float fuse; bool active,enemy,impact; } Grenade;
typedef struct { Vector3 pos; bool active; float beep; } C4;
typedef struct { Vector3 p,v; float life,maxLife,size; Color c; unsigned char kind; bool active; } Part;
typedef struct { Vector3 a,b; float life; Color c; } Tracer;
typedef struct {
    int idx,total,rounds,round,spawned,killed,stage; float time,par,roundDelay; bool roundPending;
    Vector3 start; float cacheHP; bool cacheAlive; char name[40],district[32];
    int startR,startC; bool free;
} Mission;
typedef struct {
    float move_x,move_y,look_x,look_y;
    bool fire,aim,sprint,firePressed,jump,reload,grenade,c4,det,swap,pause,map,use,brake;
} Input;
enum { SCR_SPLASH,SCR_REGISTER,SCR_LOGIN,SCR_MENU,SCR_MISSIONS,SCR_BRIEF,SCR_CHARS,SCR_SETTINGS,
       SCR_HELP,SCR_CREDITS,SCR_GAME,SCR_PAUSE,SCR_COMPLETE,SCR_OVER };
typedef struct {
    int screen,prevScreen,selMission;
    float camYaw,camPitch,camDist,fovCur,shake,hurtT,hurtAng,hitT; bool hitHead;
    float flashWhite,flashT; Vector3 flashPos;
    char banner[80]; float bannerT; Color bannerC;
    char feed[5][48]; float feedT[5]; Color feedC[5];
    bool mapBig; float completeT,warnGren,playClock; float detect; int detState;
    int rKills,rHeads,rScore,rStars,rNewChar,rCiv; float rTime; bool rRank;
} GameState;
extern Player P; extern Enemy EN[MAX_ENEMY]; extern Pickup PK[MAX_PICKUP];
extern Grenade GR[MAX_GREN]; extern C4 C4S[MAX_C4]; extern Part PT[MAX_PART]; extern Tracer TR[MAX_TRACER];
extern Mission M; extern GameState G; extern Input IN; extern Camera3D CAM;
extern const char *MISSION_NAMES[MAX_MISSIONS];
void GameInit(void);
void GameStartMission(int idx);
void GameUpdate(float dt);
void GameDraw3D(void);
void GameBanner(const char *s,Color c,float t);
void GameFeed(const char *s,Color c);
int  GameAlive(void);
bool GameObjective(Vector3 *pos,char *label,int n);
void GameEndMission(bool won);
int  MissionEnemyTotal(int idx);
int  MissionRounds(int idx);
const char *MissionDistrict(int idx);

/* ---------- city life: citizens, traffic, driving (city.c) ---------- */
extern Citizen CIT[MAX_CIT]; extern Vehicle VEH[MAX_VEH];
void CityReset(void);
void CityUpdate(float dt);
void CityDraw(Vector3 cam,Vector3 fwd);
void CityAlarm(Vector3 at,float radius);
void CityExplosion(Vector3 p,float radius,float dmg,bool byPlayer);
bool CityHitScan(Vector3 o,Vector3 d,float maxT,float *t,int *kind,int *idx);   /* kind 1 citizen, 2 vehicle */
void CityDamage(int kind,int idx,float dmg,Vector3 dir,bool byPlayer);
void CityUse(void);
void CityDrive(float dt);
bool CityNearVehicle(void);
int  SignalState(int axis);
void DrawVehicleAt(int type,Vector3 pos,float yaw,Color col,bool braking);
void GameExplode(Vector3 p,float radius,float dmg,bool byPlayer);
void GameHurtEnemy(int i,float dmg,bool head,Vector3 knock,bool byPlayer,bool expl);
void GameSmoke(Vector3 p);
void GameHurtPlayer(float dmg,Vector3 from);      /* accidents: crashes, run-overs, falls */
void GameSpawnCop(Vector3 pos);                   /* police officer on foot */

/* ---------- crime / police ---------- */
extern bool gPoliceSeen;                          /* a cop or cruiser has eyes on the player */
int CityPoliceCars(Vector3 *out,int max);         /* active police cruiser positions (minimap) */
float CityPoliceNear(void);                       /* distance to nearest cruiser, 1e9 if none */
extern int gWanted; extern float gHeat;           /* wanted stars 0..5, seconds of heat left */
void CityCrime(int stars,const char *why);

/* ---------- ui / input ---------- */
void UiInit(void);
void UiFrame(float dt);          /* polls input, updates screens */
void UiDraw(void);               /* draws everything for current screen */
void UiFree(void);
bool UiTouchMode(void);
extern float uiScale,uiW;
extern bool gQuit;
#endif
