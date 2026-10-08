/* minimal raylib API stub - compile check only */
#ifndef RAYLIB_H
#define RAYLIB_H
#include <stdbool.h>
#define PI 3.14159265358979323846f
#define DEG2RAD (PI/180.0f)
#define RAD2DEG (180.0f/PI)
typedef struct Vector2{float x,y;}Vector2; typedef struct Vector3{float x,y,z;}Vector3;
typedef struct Color{unsigned char r,g,b,a;}Color; typedef struct Rectangle{float x,y,width,height;}Rectangle;
typedef struct Texture{unsigned int id;int width,height,mipmaps,format;}Texture; typedef Texture Texture2D;
typedef struct Image{void*data;int width,height,mipmaps,format;}Image;
typedef struct Camera3D{Vector3 position,target,up;float fovy;int projection;}Camera3D;
typedef struct Camera2D{Vector2 offset,target;float rotation,zoom;}Camera2D;
typedef struct Wave{unsigned int frameCount,sampleRate,sampleSize,channels;void*data;}Wave;
typedef struct Sound{void*a;void*b;unsigned int frameCount;}Sound;
typedef struct Font{int baseSize,glyphCount,glyphPadding;Texture2D texture;}Font;
typedef struct Matrix{float m0,m4,m8,m12,m1,m5,m9,m13,m2,m6,m10,m14,m3,m7,m11,m15;}Matrix;
enum{CAMERA_PERSPECTIVE=0}; enum{FLAG_VSYNC_HINT=0x40,FLAG_MSAA_4X_HINT=0x20,FLAG_WINDOW_RESIZABLE=4};
enum{TEXTURE_FILTER_BILINEAR=1,TEXTURE_FILTER_TRILINEAR=2}; enum{TEXTURE_WRAP_REPEAT=0};
enum{MOUSE_BUTTON_LEFT=0,MOUSE_BUTTON_RIGHT=1};
enum{KEY_NULL=0,KEY_W=87,KEY_A=65,KEY_S=83,KEY_D=68,KEY_Q=81,KEY_R=82,KEY_G=71,KEY_B=66,KEY_V=86,KEY_M=77,KEY_P=80,KEY_ONE=49,KEY_TWO=50,KEY_SPACE=32,KEY_ESCAPE=256,KEY_ENTER=257,KEY_TAB=258,KEY_BACKSPACE=259,KEY_RIGHT=262,KEY_LEFT=263,KEY_DOWN=264,KEY_UP=265,KEY_F11=300,KEY_LEFT_SHIFT=340,KEY_KP_ENTER=335,KEY_F=70};
#define WHITE ((Color){255,255,255,255})
#define RED ((Color){230,41,55,255})
void InitWindow(int,int,const char*);void CloseWindow(void);bool WindowShouldClose(void);void SetConfigFlags(unsigned);
void SetTargetFPS(int);float GetFrameTime(void);double GetTime(void);int GetFPS(void);int GetScreenWidth(void);int GetScreenHeight(void);
void SetWindowMinSize(int,int);void SetExitKey(int);void SetRandomSeed(unsigned);int GetRandomValue(int,int);
void ToggleFullscreen(void);bool IsWindowFocused(void);void DisableCursor(void);void EnableCursor(void);bool IsCursorHidden(void);
void BeginDrawing(void);void EndDrawing(void);void ClearBackground(Color);void BeginMode2D(Camera2D);void EndMode2D(void);
void BeginScissorMode(int,int,int,int);void EndScissorMode(void);
bool IsKeyPressed(int);bool IsKeyDown(int);int GetKeyPressed(void);int GetCharPressed(void);
int GetMouseX(void);int GetMouseY(void);Vector2 GetMouseDelta(void);bool IsMouseButtonDown(int);bool IsMouseButtonPressed(int);float GetMouseWheelMove(void);
int GetTouchPointCount(void);Vector2 GetTouchPosition(int);int GetTouchPointId(int);
void DrawRectangle(int,int,int,int,Color);void DrawRectangleRec(Rectangle,Color);void DrawRectangleRounded(Rectangle,float,int,Color);
void DrawRectangleLinesEx(Rectangle,float,Color);void DrawRectangleGradientV(int,int,int,int,Color,Color);void DrawRectangleGradientH(int,int,int,int,Color,Color);
void DrawCircle(int,int,float,Color);void DrawCircleV(Vector2,float,Color);void DrawCircleLines(int,int,float,Color);void DrawCircleSector(Vector2,float,float,float,int,Color);
void DrawLine(int,int,int,int,Color);void DrawLineEx(Vector2,Vector2,float,Color);void DrawTriangle(Vector2,Vector2,Vector2,Color);
void DrawText(const char*,int,int,int,Color);int MeasureText(const char*,int);const char*TextFormat(const char*,...);Font GetFontDefault(void);
void DrawTexturePro(Texture2D,Rectangle,Rectangle,Vector2,float,Color);Vector2 GetWorldToScreen(Vector3,Camera3D);
void DrawSphereEx(Vector3,float,int,int,Color);void DrawCylinderEx(Vector3,Vector3,float,float,int,Color);
Image GenImageColor(int,int,Color);void ImageDrawRectangle(Image*,int,int,int,int,Color);Texture2D LoadTextureFromImage(Image);void UnloadImage(Image);
void UnloadTexture(Texture2D);void SetTextureFilter(Texture2D,int);void SetTextureWrap(Texture2D,int);void GenTextureMipmaps(Texture2D*);
void InitAudioDevice(void);void CloseAudioDevice(void);bool IsAudioDeviceReady(void);void SetMasterVolume(float);
Sound LoadSoundFromWave(Wave);void UnloadSound(Sound);void SetSoundVolume(Sound,float);void SetSoundPitch(Sound,float);void PlaySound(Sound);
bool CheckCollisionPointRec(Vector2,Rectangle);

void UpdateCamera(Camera3D*,int);void UpdateCameraPro(Camera3D*,Vector3,Vector3,float);Color Fade(Color,float);Color ColorAlpha(Color,float);Color GetColor(unsigned);
void DrawFPS(int,int);void TakeScreenshot(const char*);void OpenURL(const char*);bool FileExists(const char*);bool DirectoryExists(const char*);
void DrawRing(Vector2,float,float,float,float,int,Color);void DrawPoly(Vector2,int,float,float,Color);void DrawLine3D(Vector3,Vector3,Color);void DrawCube(Vector3,float,float,float,Color);
void DrawSphere(Vector3,float,Color);void DrawCylinder(Vector3,float,float,float,int,Color);void DrawGrid(int,float);void DrawPlane(Vector3,Vector2,Color);
void BeginMode3D(Camera3D);void EndMode3D(void);void BeginBlendMode(int);void EndBlendMode(void);void PlayMusicStream(int);void StopSound(Sound);
int TextLength(const char*);const char*TextToUpper(const char*);void TextCopy(char*,const char*);bool IsKeyReleased(int);int GetKeyPressedX(void);
Vector2 GetMousePosition(void);int GetRenderWidth(void);int GetRenderHeight(void);void SetWindowSize(int,int);void SetWindowTitle(const char*);
#endif
