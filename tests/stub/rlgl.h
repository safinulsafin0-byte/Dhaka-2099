#ifndef RLGL_H
#define RLGL_H
#define RL_QUADS 0x0007
#define RL_PROJECTION 0x1701
#define RL_MODELVIEW 0x1700
void rlBegin(int);void rlEnd(void);void rlVertex3f(float,float,float);void rlColor4ub(unsigned char,unsigned char,unsigned char,unsigned char);
void rlTexCoord2f(float,float);void rlSetTexture(unsigned int);unsigned int rlGetTextureIdDefault(void);int rlCheckRenderBatchLimit(int);
void rlPushMatrix(void);void rlPopMatrix(void);void rlTranslatef(float,float,float);void rlScalef(float,float,float);void rlRotatef(float,float,float,float);
void rlMatrixMode(int);void rlLoadIdentity(void);void rlFrustum(double,double,double,double,double,double);void rlMultMatrixf(const float*);
void rlDrawRenderBatchActive(void);void rlEnableDepthTest(void);void rlDisableDepthTest(void);
#endif
