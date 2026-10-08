#ifndef UI_INTERNAL_H
#define UI_INTERNAL_H
#include "game.h"
#define COL_BG    ((Color){9,15,20,255})
#define COL_PANEL ((Color){15,25,33,235})
#define COL_GREEN ((Color){0,166,118,255})
#define COL_RED   ((Color){244,42,65,255})
#define COL_GOLD  ((Color){255,200,60,255})
#define COL_TEXT  ((Color){236,241,243,255})
#define COL_DIM   ((Color){138,154,166,255})
typedef struct { int id; Vector2 pos; bool pressed; } Ptr;
extern Ptr uiPtr[10]; extern int uiNPtr; extern bool uiTapped; extern Vector2 uiTapPos, uiMouse;
void TextC(const char *t,float cx,float y,int sz,Color c);
void TextR(const char *t,float rx,float y,int sz,Color c);
void Panel(Rectangle r,Color c);
bool Btn(Rectangle r,const char *label,int style);   /* 0 primary 1 secondary 2 danger */
void Tri(Vector2 a,Vector2 b,Vector2 c,Color col);
void HudDraw(void);
void HudTouchInput(void);
bool HudMapOpen(void);
#endif
