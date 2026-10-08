/* Procedural sound effects - no audio files required */
#include "game.h"
#define SRATE 22050
#define POOL 4
static Sound snd[SFX_COUNT][POOL];
static int poolIdx[SFX_COUNT];
static bool audioOk=false;
Vector3 gListenPos; float gListenYaw;

/* private noise for new sounds so the shared RNG sequence (worldgen / AI) is not disturbed */
static float PNoise(void){ static unsigned int x=2463534242u; x^=x<<13; x^=x>>17; x^=x<<5; return ((float)(x&0xFFFF)/32767.5f)-1.0f; }
static float Noise(void){ return (float)GetRandomValue(-10000,10000)/10000.0f; }

static Wave Synth(int id){
    float dur=0.2f;
    switch(id){
        case SFX_GLOCK: dur=0.22f; break; case SFX_SMG: dur=0.18f; break; case SFX_RIFLE: dur=0.35f; break;
        case SFX_SHOTGUN: dur=0.5f; break; case SFX_SNIPER: dur=0.7f; break; case SFX_EXPLODE: dur=1.7f; break;
        case SFX_RELOAD: dur=0.6f; break; case SFX_PICK: dur=0.22f; break; case SFX_HIT: dur=0.05f; break;
        case SFX_HURT: dur=0.25f; break; case SFX_CLICK: dur=0.04f; break; case SFX_BEEP: dur=0.1f; break;
        case SFX_PIN: dur=0.09f; break; case SFX_EMPTY: dur=0.06f; break; case SFX_HEAD: dur=0.09f; break;
        case SFX_DEAGLE: dur=0.32f; break; case SFX_LAUNCH: dur=0.45f; break; case SFX_ALERT: dur=0.22f; break;
        case SFX_SIREN: dur=3.0f; break; case SFX_RADIO: dur=0.6f; break;
    }
    int n=(int)(dur*SRATE);
    Wave w={0}; w.frameCount=n; w.sampleRate=SRATE; w.sampleSize=16; w.channels=1;
    short *d=(short*)malloc(sizeof(short)*n); w.data=d;
    float lp=0, ph=0;
    for(int i=0;i<n;i++){
        float t=(float)i/SRATE, s=0;
        float a=0.4f,dec=15,f=120,amp=0.9f; bool gun=true;
        switch(id){
            case SFX_GLOCK: a=0.55f;dec=22;f=180; break;  case SFX_SMG: a=0.5f;dec=26;f=150; break;
            case SFX_RIFLE: a=0.35f;dec=14;f=110; break;  case SFX_SHOTGUN: a=0.3f;dec=10;f=80; break;
            case SFX_SNIPER: a=0.25f;dec=8;f=70; break;
            case SFX_DEAGLE: a=0.42f;dec=11;f=95; break;
            default: gun=false; break;
        }
        if(gun){
            lp+=a*(Noise()-lp); float env=expf(-t*dec);
            s=lp*env*amp*1.8f + sinf(6.2832f*f*t)*expf(-t*dec*1.6f)*0.5f;
            if(i<40) s+=Noise()*0.6f*(1.0f-i/40.0f);
        } else if(id==SFX_LAUNCH){
            lp+=0.12f*(Noise()-lp); s=lp*expf(-t*9)*1.6f+sinf(6.2832f*(140-170*t)*t)*expf(-t*8)*0.9f;
        } else if(id==SFX_ALERT){
            float fr=(t<0.09f)?880:1320; s=sinf(6.2832f*fr*t)*0.5f*(1.0f-t/dur);
        } else if(id==SFX_SIREN){
            /* 2s slow wail (2 sweeps) then 1s fast yelp (4 sweeps) - starts/ends at the same pitch so it loops cleanly */
            float per=(t<2.0f)?1.0f:0.25f, tt=(t<2.0f)?t:(t-2.0f);
            float fr=1000.0f+450.0f*sinf(6.2832f*tt/per-1.5708f);
            ph+=6.2832f*fr/SRATE;
            float e=fminf(1.0f,fminf(t/0.02f,(dur-t)/0.02f));
            s=(sinf(ph)*0.55f+sinf(2*ph)*0.25f+sinf(3*ph)*0.12f)*0.8f*e;
        } else if(id==SFX_RADIO){
            /* squelch chirp, static burst with gating, closing chirp */
            float g=(fmodf(t*23.0f,1.0f)<0.6f)?1.0f:0.35f; lp+=0.5f*(PNoise()-lp);
            s=0;
            if(t<0.07f) s=sinf(6.2832f*1500*t)*0.45f;
            else if(t<0.50f) s=lp*0.55f*g*(1.0f-(t-0.07f)/0.6f)+sinf(6.2832f*(480+60*sinf(t*40))*t)*0.08f;
            else s=sinf(6.2832f*1100*t)*0.4f*(1.0f-(t-0.50f)/0.1f);
        } else if(id==SFX_EXPLODE){
            lp+=0.07f*(Noise()-lp); s=lp*expf(-t*2.0f)*4.0f + sinf(6.2832f*45*t)*expf(-t*2.6f)*0.7f + Noise()*0.4f*expf(-t*12);
        } else if(id==SFX_RELOAD){
            float t2=t-0.33f; float e1=expf(-t*260), e2=(t2>0)?expf(-t2*260):0;
            s=(Noise()*0.5f+sinf(6.2832f*1300*t)*0.3f)*e1 + (Noise()*0.5f+sinf(6.2832f*900*t)*0.3f)*e2;
        } else if(id==SFX_PICK){
            float fr=(t<0.1f)?660:990; s=sinf(6.2832f*fr*t)*0.5f*(1.0f-t/dur);
        } else if(id==SFX_HIT){ s=sinf(6.2832f*1800*t)*expf(-t*80); }
        else if(id==SFX_HEAD){ s=(sinf(6.2832f*2400*t)+sinf(6.2832f*3100*t))*0.35f*expf(-t*45); }
        else if(id==SFX_HURT){ s=sinf(6.2832f*90*t)*expf(-t*14)+Noise()*0.3f*expf(-t*30); }
        else if(id==SFX_CLICK){ s=sinf(6.2832f*900*t)*(1.0f-t/dur)*0.6f; }
        else if(id==SFX_BEEP){ s=sinf(6.2832f*1400*t)*(1.0f-t/dur)*0.6f; }
        else if(id==SFX_PIN){ s=(Noise()*0.5f+sinf(6.2832f*2200*t)*0.3f)*expf(-t*70); }
        else if(id==SFX_EMPTY){ s=Noise()*0.5f*expf(-t*120); }
        d[i]=(short)(Clampf(s,-1.0f,1.0f)*30000.0f);
    }
    return w;
}

void AudioInitAll(void){
    InitAudioDevice();
    audioOk=IsAudioDeviceReady();
    if(!audioOk) return;
    for(int i=0;i<SFX_COUNT;i++){
        Wave w=Synth(i);
        for(int k=0;k<POOL;k++) snd[i][k]=LoadSoundFromWave(w);
        free(w.data);
    }
    AudioApplyVolume();
}
void AudioFreeAll(void){
    if(!audioOk) return;
    SirenUpdate(0,0);
    for(int i=0;i<SFX_COUNT;i++) for(int k=0;k<POOL;k++) UnloadSound(snd[i][k]);
    CloseAudioDevice(); audioOk=false;
}
void AudioApplyVolume(void){ if(audioOk) SetMasterVolume(SV.st.masterVol); }
void Sfx(int id,float vol,float pitch){
    if(!audioOk||id<0||id>=SFX_COUNT) return;
    Sound s=snd[id][poolIdx[id]]; poolIdx[id]=(poolIdx[id]+1)%POOL;
    SetSoundVolume(s,Clampf(vol*SV.st.sfxVol,0,1)); SetSoundPitch(s,pitch); PlaySound(s);
}
static float sirenT=0;
void SirenUpdate(float vol,float dt){
    if(!audioOk) return;
    Sound s=snd[SFX_SIREN][0];
    if(vol<=0.01f){ if(sirenT>0){ StopSound(s); sirenT=0; } return; }
    SetSoundVolume(s,Clampf(vol*SV.st.sfxVol,0,1));
    if(sirenT<=0){ PlaySound(s); sirenT=2.97f; } else sirenT-=dt;
}
void Sfx3D(int id,Vector3 at,float pitch){
    float d=V3Dist(at,gListenPos), v=Clampf(1.0f-d/150.0f,0,1); v*=v;
    if(v>0.01f) Sfx(id,v,pitch);
}
