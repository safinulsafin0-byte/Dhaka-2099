/* Local profile + progress persistence */
#include "game.h"
#ifdef _WIN32
#include <direct.h>
#define MKDIR(p) _mkdir(p)
#else
#include <sys/stat.h>
#define MKDIR(p) mkdir(p,0755)
#endif
#if defined(PLATFORM_WEB)
#include <emscripten.h>
EM_JS(void,FsMountJS,(void),{ try{ FS.mkdir('/data'); }catch(e){} FS.mount(IDBFS,{},'/data'); Module.fsReady=0; FS.syncfs(true,function(err){ Module.fsReady=1; }); });
EM_JS(int,FsReadyJS,(void),{ return Module.fsReady?1:0; });
EM_JS(void,FsFlushJS,(void),{ FS.syncfs(false,function(err){}); });
void FsMount(void){ FsMountJS(); }
int FsReady(void){ return FsReadyJS(); }
#define FS_FLUSH() FsFlushJS()
#else
#define FS_FLUSH() ((void)0)
#endif
#define SAVE_MAGIC 0xD4A2099u
#define SAVE_VER 1
Save SV;

void SaveDefaults(void){
    memset(&SV,0,sizeof(SV));
    SV.magic=SAVE_MAGIC; SV.version=SAVE_VER;
    SV.st=(Settings){0.9f,0.9f,1.0f,70.0f,false,false,false,true,true,1,1};
#if defined(PLATFORM_ANDROID)
    SV.st.touch=true;
#endif
}
/* Salted, stretched FNV-1a. Fine for a local single-player profile; not a substitute for server-side auth. */
static unsigned long long Hash(unsigned long long salt,const char *pw){
    unsigned long long h=1469598103934665603ULL^salt;
    for(int round=0;round<20000;round++){
        for(const char *p=pw;*p;p++){ h^=(unsigned char)*p; h*=1099511628211ULL; }
        h^=salt+round; h*=1099511628211ULL;
    }
    return h;
}
bool SaveLoad(void){
    SaveDefaults();
    FILE *f=fopen(SAVE_FILE,"rb"); if(!f) return false;
    Save tmp; size_t n=fread(&tmp,sizeof(Save),1,f); fclose(f);
    if(n!=1||tmp.magic!=SAVE_MAGIC||tmp.version!=SAVE_VER||!tmp.exists) return false;
    SV=tmp; return true;
}
void SaveWrite(void){
    MKDIR("data");
    FILE *f=fopen(SAVE_FILE,"wb"); if(!f) return;
    fwrite(&SV,sizeof(Save),1,f); fclose(f); FS_FLUSH();
}
void SaveRegister(const char *name,const char *user,const char *pass,const char *hint){
    Settings keep=SV.st; SaveDefaults(); SV.st=keep;
    SV.exists=true;
    snprintf(SV.name,sizeof(SV.name),"%s",name); snprintf(SV.username,sizeof(SV.username),"%s",user);
    snprintf(SV.hint,sizeof(SV.hint),"%s",hint);
    SV.salt=((unsigned long long)GetRandomValue(1,0x7fffffff)<<32)^(unsigned long long)GetRandomValue(1,0x7fffffff)^(unsigned long long)(GetTime()*1000000.0);
    SV.passHash=Hash(SV.salt,pass);
    SaveWrite();
}
bool SaveCheckLogin(const char *user,const char *pass){
    if(!SV.exists) return false;
    bool u=strcmp(user,SV.username)==0;           /* hash always computed: no timing hint on username */
    bool p=Hash(SV.salt,pass)==SV.passHash;
    return u&&p;
}
