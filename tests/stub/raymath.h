#ifndef RAYMATH_H
#define RAYMATH_H
#include "raylib.h"
#include <math.h>
static inline Matrix MatrixLookAt(Vector3 a,Vector3 b,Vector3 c){(void)a;(void)b;(void)c;Matrix m={0};return m;}
static inline float*MatrixToFloat(Matrix m){static float f[16];(void)m;return f;}
static inline Vector2 Vector2Add(Vector2 a,Vector2 b){return (Vector2){a.x+b.x,a.y+b.y};}
static inline Vector2 Vector2Subtract(Vector2 a,Vector2 b){return (Vector2){a.x-b.x,a.y-b.y};}
static inline Vector2 Vector2Scale(Vector2 a,float s){return (Vector2){a.x*s,a.y*s};}
static inline float Vector2Length(Vector2 a){return sqrtf(a.x*a.x+a.y*a.y);}
static inline float Vector2Distance(Vector2 a,Vector2 b){return Vector2Length(Vector2Subtract(a,b));}
static inline Vector2 Vector2Normalize(Vector2 a){float l=Vector2Length(a);return l>0?Vector2Scale(a,1/l):a;}

static inline float Clamp(float v,float a,float b){return v<a?a:(v>b?b:v);}static inline float Lerp(float a,float b,float t){return a+(b-a)*t;}
static inline float Normalize(float v,float a,float b){return (v-a)/(b-a);}static inline float Remap(float v,float a,float b,float c,float d){return c+(v-a)/(b-a)*(d-c);}
static inline float Wrap(float v,float a,float b){return v;}static inline int FloatEquals(float a,float b){return a==b;}
static inline Vector3 Vector3Add(Vector3 a,Vector3 b){return (Vector3){a.x+b.x,a.y+b.y,a.z+b.z};}static inline Vector3 Vector3Normalize(Vector3 a){return a;}
static inline float Vector3Length(Vector3 a){return sqrtf(a.x*a.x+a.y*a.y+a.z*a.z);}static inline Vector3 Vector3Scale(Vector3 a,float s){return (Vector3){a.x*s,a.y*s,a.z*s};}
#endif
