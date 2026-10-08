#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>

#define SCREEN_W 1280
#define SCREEN_H 720
#define WORLD_W 3200
#define WORLD_H 2200
#define MAX_ENEMIES 80
#define MAX_BULLETS 180
#define MAX_PARTICLES 160
#define MAX_TERMINALS 8
#define PROFILE_FILE "data/profile.dat"

typedef enum { SCREEN_REGISTER, SCREEN_LOGIN, SCREEN_MENU, SCREEN_GAME, SCREEN_PAUSE, SCREEN_GAMEOVER } ScreenState;
typedef enum { ENEMY_DRONE, ENEMY_ROBOT } EnemyType;

typedef struct { char name[48]; char username[32]; char password[64]; bool exists; } Profile;
typedef struct { Vector2 pos, vel; float speed; float radius; int hp, maxHp; int ammo, mag; int reserve; float fireTimer; float reloadTimer; bool reloading; int level; int xp; int kills; } Player;
typedef struct { Vector2 pos, vel; int hp, maxHp; float radius; float shootTimer; EnemyType type; bool alive; } Enemy;
typedef struct { Vector2 pos, vel; float life; bool friendly; int damage; } Bullet;
typedef struct { Vector2 pos; float life; Color color; } Particle;
typedef struct { Vector2 pos; bool hacked; float progress; } Terminal;
typedef struct { Vector2 pos; float radius; bool active; } CCTV;

enum { COL_BG = 0 }; // visual palette is intentionally kept in named constants below
static const Color NEON_CYAN = {40,220,255,255};
static const Color NEON_PINK = {255,55,170,255};
static const Color NEON_GREEN = {80,255,145,255};
static const Color DARK = {9,13,24,255};
static const Color ROAD = {24,30,43,255};
static const Color BUILDING = {37,43,58,255};
static const Color BUILDING2 = {47,54,70,255};

static Profile profile;
static ScreenState screen = SCREEN_REGISTER;
static Player player;
static Enemy enemies[MAX_ENEMIES];
static Bullet bullets[MAX_BULLETS];
static Particle particles[MAX_PARTICLES];
static Terminal terminals[MAX_TERMINALS];
static CCTV cctvs[5];
static Camera2D camera;
static Vector2 mouseWorld;
static int enemyCount = 0;
static int wave = 1;
static int waveKills = 0;
static float waveTimer = 0;
static float spawnTimer = 0;
static float messageTimer = 0;
static char message[160] = "WELCOME TO DHAKA 2099";
static char inputName[48] = "";
static char inputUser[32] = "";
static char inputPass[64] = "";
static int inputField = 0;
static bool loginError = false;
static bool showPassword = false;
static bool initialized = false;

/* Local Vector2/math helpers for Raylib versions where these helpers are not exposed. */
static Vector2 V2Add(Vector2 a, Vector2 b) { return (Vector2){a.x+b.x, a.y+b.y}; }
static Vector2 V2Sub(Vector2 a, Vector2 b) { return (Vector2){a.x-b.x, a.y-b.y}; }
static Vector2 V2Scale(Vector2 v, float s) { return (Vector2){v.x*s, v.y*s}; }
static float V2Length(Vector2 v) { return sqrtf(v.x*v.x + v.y*v.y); }
static Vector2 V2Normalize(Vector2 v) { float len=V2Length(v); return (len>0.0001f)?V2Scale(v,1.0f/len):(Vector2){0,0}; }
static float V2Distance(Vector2 a, Vector2 b) { return V2Length(V2Sub(a,b)); }
static Vector2 V2Lerp(Vector2 a, Vector2 b, float t) { return (Vector2){a.x+(b.x-a.x)*t, a.y+(b.y-a.y)*t}; }
static float ClampF(float v, float min, float max) { return v<min?min:(v>max?max:v); }

static Rectangle fieldRects[3] = {{390,260,500,48},{390,325,500,48},{390,390,500,48}};

static void SaveProfile(void) {
    FILE *f = fopen(PROFILE_FILE, "wb");
    if (!f) return;
    fwrite(&profile, sizeof(Profile), 1, f);
    fclose(f);
}
static bool LoadProfile(void) {
    FILE *f = fopen(PROFILE_FILE, "rb");
    if (!f) return false;
    bool ok = fread(&profile, sizeof(Profile), 1, f) == 1;
    fclose(f);
    return ok && profile.exists;
}
static void SetMsg(const char *s) { snprintf(message, sizeof(message), "%s", s); messageTimer = 4.0f; }
static void ResetInput(void) { inputName[0]=0; inputUser[0]=0; inputPass[0]=0; inputField=0; loginError=false; }
static void AddParticle(Vector2 p, Color c) {
    for (int i=0;i<MAX_PARTICLES;i++) if (particles[i].life<=0) { particles[i].pos=p; particles[i].life=0.5f+(float)GetRandomValue(0,100)/100.0f; particles[i].color=c; return; }
}
static void SpawnBullet(Vector2 from, Vector2 dir, bool friendly, int damage) {
    for (int i=0;i<MAX_BULLETS;i++) if (bullets[i].life<=0) {
        bullets[i].pos=from; bullets[i].vel=V2Scale(V2Normalize(dir), friendly?900.0f:420.0f); bullets[i].life=1.2f; bullets[i].friendly=friendly; bullets[i].damage=damage; return;
    }
}
static void SpawnEnemy(void) {
    if (enemyCount>=MAX_ENEMIES) return;
    Vector2 p;
    int side=GetRandomValue(0,3);
    if(side==0){p=(Vector2){camera.target.x-600, camera.target.y+GetRandomValue(-500,500)};}
    else if(side==1){p=(Vector2){camera.target.x+600, camera.target.y+GetRandomValue(-500,500)};}
    else if(side==2){p=(Vector2){camera.target.x+GetRandomValue(-700,700), camera.target.y-500};}
    else {p=(Vector2){camera.target.x+GetRandomValue(-700,700), camera.target.y+500};}
    p.x=ClampF(p.x,80, WORLD_W-80); p.y=ClampF(p.y,80,WORLD_H-80);
    for(int i=0;i<MAX_ENEMIES;i++) if(!enemies[i].alive){
        enemies[i].pos=p; enemies[i].alive=true; enemies[i].type=(GetRandomValue(0,100)<55?ENEMY_DRONE:ENEMY_ROBOT);
        enemies[i].radius=(enemies[i].type==ENEMY_DRONE?18:24); enemies[i].maxHp=(enemies[i].type==ENEMY_DRONE?45:90)+wave*6; enemies[i].hp=enemies[i].maxHp; enemies[i].shootTimer=(float)GetRandomValue(20,100)/100.0f; enemyCount++; return;
    }
}
static void StartGame(void) {
    memset(enemies,0,sizeof(enemies)); memset(bullets,0,sizeof(bullets)); memset(particles,0,sizeof(particles));
    enemyCount=0; wave=1; waveKills=0; waveTimer=0; spawnTimer=0;
    player.pos=(Vector2){1550,1080}; player.hp=120; player.maxHp=120; player.ammo=30; player.mag=30; player.reserve=180; player.fireTimer=0; player.reloading=false; player.reloadTimer=0; player.level=1; player.xp=0; player.kills=0; player.speed=240; player.radius=17;
    for(int i=0;i<MAX_TERMINALS;i++){terminals[i].pos=(Vector2){350+(float)GetRandomValue(0,2500), 280+(float)GetRandomValue(0,1500)}; terminals[i].hacked=false; terminals[i].progress=0;}
    cctvs[0]=(CCTV){{500,500},210,true}; cctvs[1]=(CCTV){{2500,500},190,true}; cctvs[2]=(CCTV){{700,1750},200,true}; cctvs[3]=(CCTV){{2600,1750},220,true}; cctvs[4]=(CCTV){{1600,450},170,true};
    camera=(Camera2D){0}; camera.offset=(Vector2){SCREEN_W/2.0f,SCREEN_H/2.0f}; camera.target=player.pos; camera.zoom=1.0f;
    screen=SCREEN_GAME; SetMsg("MISSION 01: GHOSTS OF DHAKA");
}
static void LevelUp(void){ player.level++; player.maxHp+=12; player.hp=player.maxHp; player.reserve+=45; player.xp=0; SetMsg("LEVEL UP — RESISTANCE UPGRADED"); }
static void UpdateAuth(void) {
    if (screen!=SCREEN_REGISTER && screen!=SCREEN_LOGIN) return;
    int key=GetCharPressed();
    while(key>0){
        char *target=(inputField==0?inputName:(inputField==1?inputUser:inputPass));
        int max=(inputField==0?47:(inputField==1?31:63));
        if(key>=32 && key<=126 && (int)strlen(target)<max) { int n=strlen(target); target[n]=(char)key; target[n+1]=0; }
        key=GetCharPressed();
    }
    if(IsKeyPressed(KEY_BACKSPACE)){
        char *target=(inputField==0?inputName:(inputField==1?inputUser:inputPass)); int n=strlen(target); if(n>0) target[n-1]=0;
    }
    if(IsKeyPressed(KEY_TAB)) inputField=(inputField+1)%3;
    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) for(int i=0;i<3;i++) if(CheckCollisionPointRec(GetMousePosition(),fieldRects[i])) inputField=i;
    if(IsKeyPressed(KEY_ENTER)){
        if(screen==SCREEN_REGISTER){
            if(strlen(inputName)<2 || strlen(inputUser)<3 || strlen(inputPass)<3){loginError=true; return;}
            profile.exists=true; snprintf(profile.name,sizeof(profile.name),"%s",inputName); snprintf(profile.username,sizeof(profile.username),"%s",inputUser); snprintf(profile.password,sizeof(profile.password),"%s",inputPass); SaveProfile(); ResetInput(); screen=SCREEN_MENU; SetMsg("REGISTRATION COMPLETE");
        } else {
            if(!profile.exists){screen=SCREEN_REGISTER; ResetInput(); return;}
            if(strcmp(inputUser,profile.username)==0 && strcmp(inputPass,profile.password)==0){ResetInput(); screen=SCREEN_MENU; SetMsg("ACCESS GRANTED");} else loginError=true;
        }
    }
}
static void UpdateGame(void){
    if(screen!=SCREEN_GAME) return;
    float dt=GetFrameTime();
    if(IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_P)){screen=SCREEN_PAUSE;return;}
    if(IsKeyPressed(KEY_F10)){CloseWindow();return;}
    Vector2 move={0}; if(IsKeyDown(KEY_W)||IsKeyDown(KEY_UP))move.y-=1; if(IsKeyDown(KEY_S)||IsKeyDown(KEY_DOWN))move.y+=1; if(IsKeyDown(KEY_A)||IsKeyDown(KEY_LEFT))move.x-=1; if(IsKeyDown(KEY_D)||IsKeyDown(KEY_RIGHT))move.x+=1;
    float speed=player.speed*(IsKeyDown(KEY_LEFT_SHIFT)||IsKeyDown(KEY_RIGHT_SHIFT)?1.55f:1.0f); if(V2Length(move)>0) player.pos=V2Add(player.pos,V2Scale(V2Normalize(move),speed*dt));
    player.pos.x=ClampF(player.pos.x,40,WORLD_W-40); player.pos.y=ClampF(player.pos.y,40,WORLD_H-40);
    camera.target=V2Lerp(camera.target,player.pos,ClampF(8*dt,0,1));
    mouseWorld=GetScreenToWorld2D(GetMousePosition(),camera);
    if(player.reloading){player.reloadTimer-=dt;if(player.reloadTimer<=0){int need=player.mag-player.ammo;int take=need<player.reserve?need:player.reserve;player.ammo+=take;player.reserve-=take;player.reloading=false;}}
    if(IsKeyPressed(KEY_R)&&!player.reloading&&player.ammo<player.mag&&player.reserve>0){player.reloading=true;player.reloadTimer=1.1f;}
    player.fireTimer-=dt;
    if((IsMouseButtonDown(MOUSE_BUTTON_LEFT)||IsKeyDown(KEY_SPACE))&&!player.reloading&&player.fireTimer<=0&&player.ammo>0){Vector2 dir=V2Sub(mouseWorld,player.pos);SpawnBullet(V2Add(player.pos,V2Scale(V2Normalize(dir),20)),dir,true,25+player.level*2);player.ammo--;player.fireTimer=0.12f;AddParticle(player.pos,NEON_CYAN);}
    if(player.ammo==0&&player.reserve>0&&!player.reloading) {player.reloading=true;player.reloadTimer=1.1f;}
    spawnTimer-=dt; int desired=4+wave*2; if(spawnTimer<=0 && enemyCount<desired && enemyCount<MAX_ENEMIES){SpawnEnemy();spawnTimer=0.65f;}
    waveTimer+=dt; if(waveKills>=desired){wave++;waveKills=0;waveTimer=0;SetMsg("WAVE CLEAR — THREAT LEVEL INCREASED");}
    for(int i=0;i<MAX_BULLETS;i++) if(bullets[i].life>0){bullets[i].pos=V2Add(bullets[i].pos,V2Scale(bullets[i].vel,dt));bullets[i].life-=dt;if(bullets[i].pos.x<0||bullets[i].pos.x>WORLD_W||bullets[i].pos.y<0||bullets[i].pos.y>WORLD_H)bullets[i].life=0;}
    for(int i=0;i<MAX_ENEMIES;i++) if(enemies[i].alive){Enemy *e=&enemies[i];Vector2 d=V2Sub(player.pos,e->pos);float dist=V2Length(d);float es=e->type==ENEMY_DRONE?100:72;if(dist>45)e->pos=V2Add(e->pos,V2Scale(V2Normalize(d),es*dt));e->shootTimer-=dt;if(dist<500&&e->shootTimer<=0){SpawnBullet(V2Add(e->pos,V2Scale(V2Normalize(d),e->radius+3)),d,false,e->type==ENEMY_DRONE?7:11);e->shootTimer=(e->type==ENEMY_DRONE?1.4f:1.9f);}}
    for(int i=0;i<MAX_BULLETS;i++) if(bullets[i].life>0){
        Bullet *b=&bullets[i];
        if(b->friendly){
            for(int j=0;j<MAX_ENEMIES;j++) if(enemies[j].alive && CheckCollisionPointCircle(b->pos,enemies[j].pos,enemies[j].radius)){
                enemies[j].hp-=b->damage;
                b->life=0;
                AddParticle(enemies[j].pos,NEON_PINK);
                if(enemies[j].hp<=0){
                    enemies[j].alive=false; enemyCount--; waveKills++; player.kills++; player.xp+=25;
                    if(player.xp>=100) LevelUp();
                }
                break;
            }
        } else if(CheckCollisionPointCircle(b->pos,player.pos,player.radius)){
            b->life=0; player.hp-=b->damage; AddParticle(player.pos,NEON_PINK);
            if(player.hp<=0){player.hp=0;screen=SCREEN_GAMEOVER;}
        }
    }
    if(IsKeyPressed(KEY_E)){for(int i=0;i<MAX_TERMINALS;i++){if(!terminals[i].hacked&&V2Distance(player.pos,terminals[i].pos)<70){terminals[i].hacked=true;player.xp+=20;SetMsg("TERMINAL HACKED — CCTV BLIND SPOT CREATED");break;}}}
    for(int i=0;i<MAX_PARTICLES;i++)if(particles[i].life>0)particles[i].life-=dt;
    if(messageTimer>0)messageTimer-=dt;
}
static void DrawAuth(bool reg){
    ClearBackground(DARK); DrawRectangle(0,0,SCREEN_W,SCREEN_H,(Color){6,10,18,255});
    for(int y=0;y<SCREEN_H;y+=40)DrawLine(0,y,SCREEN_W,y,(Color){14,25,39,255}); for(int x=0;x<SCREEN_W;x+=40)DrawLine(x,0,x,SCREEN_H,(Color){14,25,39,255});
    DrawText("DHAKA 2099",330,75,64,NEON_CYAN); DrawText("CYBERPUNK SURVIVAL",380,145,25,NEON_PINK);
    DrawText(reg?"RESISTANCE REGISTRATION":"RESISTANCE LOGIN",440,205,25,RAYWHITE);
    const char *labels[3]={"FULL NAME","USERNAME","PASSWORD"}; char *vals[3]={inputName,inputUser,inputPass};
    for(int i=0;i<3;i++){DrawText(labels[i],fieldRects[i].x,fieldRects[i].y-22,16,(i==inputField?NEON_CYAN:LIGHTGRAY));DrawRectangleLinesEx(fieldRects[i],2,(i==inputField?NEON_CYAN:(Color){80,90,110,255}));char shown[70];if(i==2&&!showPassword){int n=strlen(vals[i]);for(int k=0;k<n;k++)shown[k]='*';shown[n]=0;}else snprintf(shown,sizeof(shown),"%s",vals[i]);DrawText(shown,fieldRects[i].x+15,fieldRects[i].y+13,20,RAYWHITE);}
    DrawText("TAB: next field    ENTER: confirm",420,470,18,GRAY); DrawText("F10: exit",560,505,16,GRAY);
    if(loginError)DrawText(reg?"Please complete all fields.":"Invalid username or password.",440,545,18,NEON_PINK);
}
static void DrawMenu(void){
    ClearBackground(DARK); DrawText("DHAKA 2099",65,55,56,NEON_CYAN);DrawText("CYBERPUNK SURVIVAL",70,118,24,NEON_PINK);
    DrawText(TextFormat("OPERATIVE: %s",profile.name),70,175,22,RAYWHITE);DrawText(TextFormat("@%s",profile.username),70,205,18,GRAY);
    DrawRectangle(70,260,460,220,(Color){16,23,36,255});DrawRectangleLines(70,260,460,220,NEON_CYAN);
    DrawText("MAIN OPERATIONS",100,285,22,NEON_CYAN);DrawText("[ ENTER ]  START MISSION",105,335,22,RAYWHITE);DrawText("[ L ]       LOG OUT",105,380,22,RAYWHITE);DrawText("[ F10 ]     EXIT",105,425,22,RAYWHITE);
    DrawText("2099 // DHAKA MEGACITY",760,270,26,NEON_PINK);DrawText("AI-CONTROLLED ZONES",760,310,20,GRAY);DrawText("RESISTANCE NETWORK ONLINE",760,345,20,NEON_GREEN);DrawText("MISSION STATUS: READY",760,380,20,RAYWHITE);
    DrawText("Move  •  Shoot  •  Hack  •  Survive",760,500,20,GRAY);
    if(messageTimer>0)DrawText(message,70,650,18,NEON_GREEN);
}
static void DrawWorld(void){
    BeginMode2D(camera);ClearBackground((Color){7,12,20,255});
    // city grid / roads
    for(int x=0;x<WORLD_W;x+=160)DrawRectangle(x,0,70,WORLD_H,ROAD); for(int y=0;y<WORLD_H;y+=160)DrawRectangle(0,y,WORLD_W,70,ROAD);
    for(int x=90;x<WORLD_W;x+=160)DrawRectangle(x,0,3,WORLD_H,(Color){42,55,72,255}); for(int y=90;y<WORLD_H;y+=160)DrawRectangle(0,y,WORLD_W,3,(Color){42,55,72,255});
    // buildings
    for(int gx=0;gx<WORLD_W;gx+=320)for(int gy=0;gy<WORLD_H;gy+=320){Rectangle r={gx+90,gy+90,185,185};DrawRectangleRec(r,BUILDING);DrawRectangleLinesEx(r,2,BUILDING2);for(int wx=0;wx<4;wx++)for(int wy=0;wy<4;wy++)DrawRectangle(r.x+20+wx*40,r.y+20+wy*40,12,18,(Color){70,95,115,255});}
    // CCTV fields
    for(int i=0;i<5;i++)if(cctvs[i].active){DrawCircleLinesV(cctvs[i].pos,cctvs[i].radius,(Color){255,50,100,90});DrawCircleV(cctvs[i].pos,6,NEON_PINK);}
    // terminals
    for(int i=0;i<MAX_TERMINALS;i++){Color c=terminals[i].hacked?NEON_GREEN:NEON_CYAN;DrawRectangle(terminals[i].pos.x-12,terminals[i].pos.y-16,24,32,c);DrawRectangleLines(terminals[i].pos.x-18,terminals[i].pos.y-22,36,44,c);}
    // bullets
    for(int i=0;i<MAX_BULLETS;i++)if(bullets[i].life>0)DrawCircleV(bullets[i].pos,bullets[i].friendly?4:5,bullets[i].friendly?NEON_CYAN:NEON_PINK);
    // enemies
    for(int i=0;i<MAX_ENEMIES;i++)if(enemies[i].alive){Enemy *e=&enemies[i];Color c=e->type==ENEMY_DRONE?NEON_PINK:(Color){255,130,50,255};DrawCircleV(e->pos,e->radius,(Color){30,35,50,255});DrawCircleLinesV(e->pos,e->radius,c);if(e->type==ENEMY_DRONE){DrawLine(e->pos.x-10,e->pos.y,e->pos.x+10,e->pos.y,c);DrawLine(e->pos.x,e->pos.y-10,e->pos.x,e->pos.y+10,c);}else DrawRectangle(e->pos.x-10,e->pos.y-10,20,20,c);DrawRectangle(e->pos.x-25,e->pos.y-e->radius-10,50,5,DARK);DrawRectangle(e->pos.x-25,e->pos.y-e->radius-10,50*((float)e->hp/e->maxHp),5,NEON_GREEN);}
    // player
    Vector2 aim=V2Normalize(V2Sub(mouseWorld,player.pos));DrawLineEx(player.pos,V2Add(player.pos,V2Scale(aim,32)),7,NEON_CYAN);DrawCircleV(player.pos,player.radius,(Color){18,35,50,255});DrawCircleLinesV(player.pos,player.radius,NEON_GREEN);DrawCircleV(player.pos,5,RAYWHITE);
    for(int i=0;i<MAX_PARTICLES;i++)if(particles[i].life>0)DrawCircleV(particles[i].pos,3+particles[i].life*3,particles[i].color);
    EndMode2D();
}
static void DrawHUD(void){
    DrawRectangle(0,0,SCREEN_W,76,(Color){5,9,16,235});DrawText("DHAKA 2099",22,16,26,NEON_CYAN);DrawText(TextFormat("LEVEL %d",player.level),220,20,20,NEON_GREEN);DrawText(TextFormat("WAVE %d",wave),330,20,20,NEON_PINK);DrawText(TextFormat("KILLS %d",player.kills),440,20,20,RAYWHITE);
    DrawText("HP",600,15,16,GRAY);DrawRectangle(630,16,180,16,DARK);DrawRectangle(630,16,180*((float)player.hp/player.maxHp),16,NEON_GREEN);DrawText(TextFormat("%d/%d",player.hp,player.maxHp),820,14,16,RAYWHITE);
    DrawText(TextFormat("AMMO %02d / %03d",player.ammo,player.reserve),980,17,20,RAYWHITE);if(player.reloading)DrawText("RELOADING",980,43,14,NEON_PINK);
    DrawText("WASD Move   SHIFT Sprint   LMB/SPACE Fire   R Reload   E Hack   P Pause",22,690,16,GRAY);
    if(messageTimer>0)DrawText(message,SCREEN_W/2-MeasureText(message,20)/2,92,20,NEON_GREEN);
    // minimap
    Rectangle mini={SCREEN_W-190,SCREEN_H-185,160,130};DrawRectangleRec(mini,(Color){5,9,15,230});DrawRectangleLinesEx(mini,2,NEON_CYAN);float sx=mini.width/WORLD_W,sy=mini.height/WORLD_H;DrawCircle(mini.x+player.pos.x*sx,mini.y+player.pos.y*sy,4,NEON_GREEN);for(int i=0;i<MAX_ENEMIES;i++)if(enemies[i].alive)DrawCircle(mini.x+enemies[i].pos.x*sx,mini.y+enemies[i].pos.y*sy,2,NEON_PINK);DrawText("CITY MAP",mini.x+8,mini.y+8,12,GRAY);
}
static void DrawPause(void){DrawRectangle(0,0,SCREEN_W,SCREEN_H,(Color){0,0,0,170});DrawText("MISSION PAUSED",470,260,42,RAYWHITE);DrawText("ENTER / P / ESC  Resume",465,330,22,NEON_CYAN);DrawText("F10  Exit",570,370,18,GRAY);}
static void DrawGameOver(void){ClearBackground(DARK);DrawText("RESISTANCE SIGNAL LOST",330,220,45,NEON_PINK);DrawText(TextFormat("Operative %s",profile.name),500,295,24,RAYWHITE);DrawText(TextFormat("KILLS: %d     LEVEL: %d",player.kills,player.level),445,340,22,GRAY);DrawText("ENTER  Restart Mission",470,420,22,NEON_CYAN);DrawText("ESC  Return to Main Menu",440,460,20,GRAY);}
int main(void){
    InitWindow(SCREEN_W,SCREEN_H,"Dhaka 2099 — Cyberpunk Survival");SetTargetFPS(60);SetExitKey(KEY_NULL);initialized=true;
    bool has=LoadProfile();screen=has?SCREEN_LOGIN:SCREEN_REGISTER;ResetInput();
    while(!WindowShouldClose()){
        if(screen==SCREEN_REGISTER||screen==SCREEN_LOGIN)UpdateAuth();
        else if(screen==SCREEN_MENU){if(IsKeyPressed(KEY_ENTER))StartGame();if(IsKeyPressed(KEY_L)){screen=SCREEN_LOGIN;ResetInput();}if(IsKeyPressed(KEY_F10))break;}
        else if(screen==SCREEN_GAME)UpdateGame();
        else if(screen==SCREEN_PAUSE){if(IsKeyPressed(KEY_P)||IsKeyPressed(KEY_ESCAPE)||IsKeyPressed(KEY_ENTER))screen=SCREEN_GAME;if(IsKeyPressed(KEY_F10))break;}
        else if(screen==SCREEN_GAMEOVER){if(IsKeyPressed(KEY_ENTER))StartGame();if(IsKeyPressed(KEY_ESCAPE))screen=SCREEN_MENU;if(IsKeyPressed(KEY_F10))break;}
        BeginDrawing();
        if(screen==SCREEN_REGISTER)DrawAuth(true); else if(screen==SCREEN_LOGIN)DrawAuth(false); else if(screen==SCREEN_MENU)DrawMenu(); else if(screen==SCREEN_GAME){DrawWorld();DrawHUD();} else if(screen==SCREEN_PAUSE){DrawWorld();DrawHUD();DrawPause();} else if(screen==SCREEN_GAMEOVER)DrawGameOver();
        EndDrawing();
    }
    CloseWindow();return 0;
}
