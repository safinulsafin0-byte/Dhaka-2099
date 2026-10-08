#include "game.h"
#if defined(PLATFORM_WEB)
#include <emscripten/emscripten.h>
#endif
#if defined(PLATFORM_WEB)
void FsMount(void); int FsReady(void);
#endif
static bool booted=false;
static void Frame(void){
#if defined(PLATFORM_WEB)
    if(!booted){ if(!FsReady()){ BeginDrawing(); ClearBackground((Color){9,15,20,255}); DrawText("LOADING...",40,40,30,WHITE); EndDrawing(); return; } UiInit(); AudioInitAll(); booted=true; }
#endif
    UiFrame(GetFrameTime()); UiDraw();
}
int main(void){
    SetConfigFlags(FLAG_MSAA_4X_HINT|FLAG_VSYNC_HINT|FLAG_WINDOW_RESIZABLE);
    InitWindow(1280,720,GAME_TITLE); SetWindowMinSize(640,360); SetExitKey(KEY_NULL);
    SetRandomSeed((unsigned int)(GetTime()*1000000.0)+12345u);
#if defined(PLATFORM_WEB)
    FsMount(); emscripten_set_main_loop(Frame,0,1);
#else
    UiInit(); AudioInitAll(); booted=true;
    SetTargetFPS(144);
    while(!WindowShouldClose()&&!gQuit) Frame();
#endif
    if(SV.exists) SaveWrite();
    UiFree(); AudioFreeAll(); CloseWindow();
    return 0;
}
