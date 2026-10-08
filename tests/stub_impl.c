#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
void InitWindow(int a,int b,const char*c){(void)a;(void)b;(void)c;}void CloseWindow(void){}bool WindowShouldClose(void){return false;}void SetConfigFlags(unsigned a){(void)a;}
void SetTargetFPS(int a){(void)a;}float GetFrameTime(void){return 1/60.0f;}double GetTime(void){static double t=0;t+=1/60.0;return t;}int GetFPS(void){return 60;}
int GetScreenWidth(void){return 1280;}int GetScreenHeight(void){return 720;}void SetWindowMinSize(int a,int b){(void)a;(void)b;}void SetExitKey(int a){(void)a;}
void SetRandomSeed(unsigned s){srand(s);}int GetRandomValue(int a,int b){return a+rand()%(b-a+1);}
void ToggleFullscreen(void){}bool IsWindowFocused(void){return true;}void DisableCursor(void){}void EnableCursor(void){}bool IsCursorHidden(void){return false;}
void BeginDrawing(void){}void EndDrawing(void){}void ClearBackground(Color c){(void)c;}void BeginMode2D(Camera2D c){(void)c;}void EndMode2D(void){}
void BeginScissorMode(int a,int b,int c,int d){(void)a;(void)b;(void)c;(void)d;}void EndScissorMode(void){}
int stubKey=0; bool IsKeyPressed(int k){return k==stubKey;}bool IsKeyDown(int k){(void)k;return false;}int GetKeyPressed(void){return 0;}int GetCharPressed(void){return 0;}
int GetMouseX(void){return 0;}int GetMouseY(void){return 0;}Vector2 GetMouseDelta(void){return (Vector2){0,0};}bool IsMouseButtonDown(int b){(void)b;return false;}
bool IsMouseButtonPressed(int b){(void)b;return false;}float GetMouseWheelMove(void){return 0;}
int GetTouchPointCount(void){return 0;}Vector2 GetTouchPosition(int i){(void)i;return (Vector2){0,0};}int GetTouchPointId(int i){return i;}
void DrawRectangle(int a,int b,int c,int d,Color e){(void)a;(void)b;(void)c;(void)d;(void)e;}void DrawRectangleRec(Rectangle r,Color c){(void)r;(void)c;}
void DrawRectangleRounded(Rectangle r,float f,int s,Color c){(void)r;(void)f;(void)s;(void)c;}void DrawRectangleLinesEx(Rectangle r,float t,Color c){(void)r;(void)t;(void)c;}
void DrawRectangleGradientV(int a,int b,int c,int d,Color e,Color f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;}
void DrawRectangleGradientH(int a,int b,int c,int d,Color e,Color f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;}
void DrawCircle(int a,int b,float c,Color d){(void)a;(void)b;(void)c;(void)d;}void DrawCircleV(Vector2 a,float b,Color c){(void)a;(void)b;(void)c;}
void DrawCircleLines(int a,int b,float c,Color d){(void)a;(void)b;(void)c;(void)d;}void DrawCircleSector(Vector2 a,float b,float c,float d,int e,Color f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;}void DrawLine(int a,int b,int c,int d,Color e){(void)a;(void)b;(void)c;(void)d;(void)e;}
void DrawLineEx(Vector2 a,Vector2 b,float c,Color d){(void)a;(void)b;(void)c;(void)d;}void DrawTriangle(Vector2 a,Vector2 b,Vector2 c,Color d){(void)a;(void)b;(void)c;(void)d;}
void DrawText(const char*t,int a,int b,int c,Color d){(void)t;(void)a;(void)b;(void)c;(void)d;}int MeasureText(const char*t,int s){return (int)strlen(t)*s/2;}
const char*TextFormat(const char*f,...){static char b[4][256];static int i=0;i=(i+1)&3;va_list a;va_start(a,f);vsnprintf(b[i],256,f,a);va_end(a);return b[i];}
Font GetFontDefault(void){Font f={0};return f;}void DrawTexturePro(Texture2D t,Rectangle a,Rectangle b,Vector2 c,float d,Color e){(void)t;(void)a;(void)b;(void)c;(void)d;(void)e;}
Vector2 GetWorldToScreen(Vector3 p,Camera3D c){(void)p;(void)c;return (Vector2){640,360};}
void DrawSphereEx(Vector3 a,float b,int c,int d,Color e){(void)a;(void)b;(void)c;(void)d;(void)e;}void DrawCylinderEx(Vector3 a,Vector3 b,float c,float d,int e,Color f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;}
Image GenImageColor(int w,int h,Color c){Image i={0};i.width=w;i.height=h;i.data=malloc((size_t)w*h*4);for(int k=0;k<w*h;k++){((Color*)i.data)[k]=c;}return i;}
void ImageDrawRectangle(Image*im,int x,int y,int w,int h,Color c){ /* strict bounds check to catch overflow */
  if(x<-w||y<-h||x>im->width||y>im->height){fprintf(stderr,"IMAGE RECT OUT OF RANGE %d %d %d %d\n",x,y,w,h);}
  for(int j=y;j<y+h;j++)for(int i=x;i<x+w;i++)if(i>=0&&j>=0&&i<im->width&&j<im->height)((Color*)im->data)[j*im->width+i]=c;}
Texture2D LoadTextureFromImage(Image i){Texture2D t={1,i.width,i.height,1,0};return t;}void UnloadImage(Image i){free(i.data);}void UnloadTexture(Texture2D t){(void)t;}
void SetTextureFilter(Texture2D t,int f){(void)t;(void)f;}void SetTextureWrap(Texture2D t,int f){(void)t;(void)f;}void GenTextureMipmaps(Texture2D*t){(void)t;}
void InitAudioDevice(void){}void CloseAudioDevice(void){}bool IsAudioDeviceReady(void){return true;}void SetMasterVolume(float v){(void)v;}
Sound LoadSoundFromWave(Wave w){(void)w;Sound s={0};return s;}void UnloadSound(Sound s){(void)s;}void SetSoundVolume(Sound s,float v){(void)s;(void)v;}void SetSoundPitch(Sound s,float v){(void)s;(void)v;}void PlaySound(Sound s){(void)s;}void StopSound(Sound s){(void)s;}
bool CheckCollisionPointRec(Vector2 p,Rectangle r){return p.x>=r.x&&p.x<=r.x+r.width&&p.y>=r.y&&p.y<=r.y+r.height;}
void rlBegin(int a){(void)a;}void rlEnd(void){}void rlVertex3f(float a,float b,float c){(void)a;(void)b;(void)c;}void rlColor4ub(unsigned char a,unsigned char b,unsigned char c,unsigned char d){(void)a;(void)b;(void)c;(void)d;}
void rlTexCoord2f(float a,float b){(void)a;(void)b;}void rlSetTexture(unsigned int a){(void)a;}unsigned int rlGetTextureIdDefault(void){return 1;}int rlCheckRenderBatchLimit(int a){(void)a;return 0;}
void rlPushMatrix(void){}void rlPopMatrix(void){}void rlTranslatef(float a,float b,float c){(void)a;(void)b;(void)c;}void rlScalef(float a,float b,float c){(void)a;(void)b;(void)c;}void rlRotatef(float a,float b,float c,float d){(void)a;(void)b;(void)c;(void)d;}
void rlMatrixMode(int a){(void)a;}void rlLoadIdentity(void){}void rlFrustum(double a,double b,double c,double d,double e,double f){(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;}void rlMultMatrixf(const float*m){(void)m;}
void rlDrawRenderBatchActive(void){}void rlEnableDepthTest(void){}void rlDisableDepthTest(void){}
