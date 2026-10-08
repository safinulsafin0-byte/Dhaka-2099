/* Procedural Dhaka: districts, landmarks, enemy base, collision, ray casting, navigation, rendering */
#include "game.h"
#define C(r,g,b) ((Color){r,g,b,255})

Obs OBS[MAX_OBS]; int nObs;
Deco DECO[MAX_DECO]; int nDeco;
Barrel BARREL[MAX_BARREL]; int nBarrel;
BaseInfo BASE;
Texture2D mapTex;
static Texture2D texWin;
static bool haveMap=false;

static const char *DMAP[BLOCKS]={
 "MMMUUUUUUGGG","MMMUUUUUUGGG","MMCUUUUGGGGG","MMMJFFYYGGGG","DDDFFFYYGHGG","DLLFFRNTTHTT",
 "DDDFRRTTTTTT","ODDFFTTTTTTT","OBOOOOOOTTTT","OOOOOAOOOOOO","OOOOOOOOOOOO","SSS~~~~~~SSS"};

static void WaterBuild(void); static void GridBuild(void);
static unsigned rs=1;
static float R01(void){ rs^=rs<<13; rs^=rs>>17; rs^=rs<<5; return (rs&0xFFFFFF)/16777216.0f; }
static float RR(float a,float b){ return a+(b-a)*R01(); }

static char CodeAt(float x,float z){
    int c=(int)(x/BLOCK_SZ), r=(int)(z/BLOCK_SZ);
    c=c<0?0:(c>=BLOCKS?BLOCKS-1:c); r=r<0?0:(r>=BLOCKS?BLOCKS-1:r);
    return DMAP[r][c];
}
const char *DistrictAt(float x,float z){
    switch(CodeAt(x,z)){
        case 'M':return "MIRPUR"; case 'U':return "UTTARA"; case 'G':return "GULSHAN"; case 'D':return "DHANMONDI";
        case 'F':return "FARMGATE"; case 'T':return "MOTIJHEEL"; case 'O':return "OLD DHAKA"; case 'R':return "RAMNA PARK";
        case 'N':return "SHAHBAG"; case 'J':return "SHER-E-BANGLA NAGAR"; case 'B':return "LALBAGH FORT"; case 'A':return "AHSAN MANZIL";
        case 'L':return "DHANMONDI LAKE"; case 'Y':return "TEJGAON"; case 'C':return "SHER-E-BANGLA STADIUM"; case 'H':return "HATIRJHEEL"; case 'S':return "SADARGHAT"; default:return "BURIGANGA RIVER";
    }
}
bool BlockValidForBase(int r,int c){ return strchr("MUGDFTOY",DMAP[r][c])!=NULL; }

static int AddObs(float x0,float y0,float z0,float x1,float y1,float z1,int kind,Color col,int style){
    if(nObs>=MAX_OBS) return -1;
    Obs *o=&OBS[nObs]; o->mn=V3(x0,y0,z0); o->mx=V3(x1,y1,z1); o->kind=kind; o->style=style; o->active=true; o->col=col;
    return nObs++;
}
static int Blk(float cx,float cz,float w,float d,float h,int kind,Color col,int style){
    return AddObs(cx-w/2,0,cz-d/2,cx+w/2,h,cz+d/2,kind,col,style);
}
static void AddDeco(int kind,float x,float y,float z,float a,float b,Color col){
    if(nDeco>=MAX_DECO) return;
    Deco *d=&DECO[nDeco++]; d->kind=kind; d->p=V3(x,y,z); d->a=a; d->b=b; d->col=col;
}
static void Quad(float cx,float cz,float w,float d,float y,Color c){ AddDeco(DK_QUAD,cx,y,cz,w,d,c); }
static void AddBarrel(float x,float z){
    if(nBarrel>=MAX_BARREL) return;
    int o=Blk(x,z,0.75f,0.75f,1.1f,OB_BARREL,C(170,40,30),nBarrel); if(o<0) return;
    BARREL[nBarrel]=(Barrel){V3(x,0,z),30,-1,true,o}; nBarrel++;
}
static void AddVehicleT(float x,float z,bool alongZ,int type);
Stall STALLS[MAX_STALL]; int nStall;
static void Tree(float x,float z,float s){ AddDeco(DK_TREE,x,0,z,s,0,C(40+(int)RR(0,40),100+(int)RR(0,50),40)); }

/* ---------- district styles ---------- */
typedef struct { float hmin,hmax,fill,fmin,fmax; Color pal[3]; bool trees; } DStyle;
static DStyle StyleFor(char c){
    switch(c){
        case 'U': return (DStyle){12,30,0.80f,32,46,{C(240,235,225),C(225,225,215),C(235,215,190)},true};
        case 'G': return (DStyle){28,85,0.70f,32,46,{C(170,200,225),C(190,215,230),C(200,205,215)},true};
        case 'D': return (DStyle){12,32,0.78f,34,48,{C(225,215,195),C(200,215,200),C(215,200,190)},true};
        case 'F': return (DStyle){14,42,0.90f,38,50,{C(230,200,150),C(210,190,200),C(200,210,220)},false};
        case 'T': return (DStyle){40,115,0.75f,34,48,{C(180,190,205),C(200,200,210),C(165,180,195)},false};
        case 'Y': return (DStyle){7,14,0.75f,40,52,{C(170,176,184),C(190,160,120),C(150,170,190)},false};
        case 'O': return (DStyle){9,20,1.00f,44,54,{C(245,170,190),C(150,215,215),C(250,225,140)},false};
        default:  return (DStyle){10,26,0.95f,36,52,{C(235,205,170),C(220,170,160),C(190,200,215)},false};
    }
}
static bool RectFree(float x0,float z0,float x1,float z1){
    for(int i=0;i<nObs;i++){ const Obs *o=&OBS[i]; if(o->kind==OB_WATER) continue;
        if(x1>o->mn.x&&x0<o->mx.x&&z1>o->mn.z&&z0<o->mx.z) return false; }
    return true;
}
static void AddStall(float x,float z,int face,int kind){
    static const Color body[4]={C(200,60,50),C(70,150,70),C(240,190,60),C(50,120,190)};
    float sw=3.0f,sd=2.2f; if(face>=2){ float t=sw; sw=sd; sd=t; }
    if(!RectFree(x-sw/2-0.8f,z-sd/2-0.8f,x+sw/2+0.8f,z+sd/2+0.8f)) return;
    if(Blk(x,z,sw,sd,2.3f,OB_SOLID,body[kind],100+kind*4+face)<0) return;
    static const float yw[4]={PI,0,-PI/2,PI/2};
    if(nStall<MAX_STALL) STALLS[nStall++]=(Stall){V3(x,0,z),yw[face],false};
}
static void Awning(float x,float z,float len,int face,Color c){ AddDeco(DK_AWNING,x,3.2f,z,len,(float)face,c); }
static void GenMosque(float cx,float cz){
    Blk(cx,cz,30,30,11,OB_SOLID,C(236,233,222),20);
    AddDeco(DK_DOME,cx,11,cz,8.0f,0,C(70,150,110));
    for(int a=-1;a<=1;a+=2) for(int b=-1;b<=1;b+=2) AddDeco(DK_POLE,cx+a*13.5f,0,cz+b*13.5f,22.0f,0,C(236,233,222));
}
static void GenCity(float x0,float z0,char code){
    DStyle st=StyleFor(code);
    Quad(x0+85,z0+85,170,170,0.12f,C(146,144,138));
    float lot=170.0f/3; bool mosque=false;
    static const Color ac[5]={{200,60,50,255},{50,120,190,255},{230,170,40,255},{60,150,90,255},{170,70,150,255}};
    for(int i=0;i<3;i++) for(int j=0;j<3;j++){
        float lx=x0+lot*(i+0.5f), lz=z0+lot*(j+0.5f);
        if(R01()>st.fill){
            if(st.trees) Tree(lx,lz,RR(0.9f,1.3f));
            else if(code=='O'||code=='M'||code=='F') for(int k=0;k<3;k++) AddVehicleT(lx+RR(-14,14),lz+RR(-14,14),R01()<0.5f,0);
            continue; }
        if(!mosque&&R01()<0.08f&&code!='T'&&code!='G'&&code!='Y'){ GenMosque(lx,lz); mosque=true; continue; }
        float w=RR(st.fmin,st.fmax), d=RR(st.fmin,st.fmax); if(w>lot-4)w=lot-4; if(d>lot-4)d=lot-4;
        float cx=lx+RR(-1,1)*(lot-w)*0.4f, cz=lz+RR(-1,1)*(lot-d)*0.4f;
        cx=Clampf(cx,x0+2.0f+w/2,x0+168.0f-w/2); cz=Clampf(cz,z0+2.0f+d/2,z0+168.0f-d/2);
        float h=RR(st.hmin,st.hmax);
        Blk(cx,cz,w,d,h,OB_BUILD,st.pal[(int)(R01()*2.999f)],(R01()<0.5f)?1:0);
        if(code!='Y'){
            if(i==0) Awning(cx-w/2-0.8f,cz+RR(-d/4,d/4),RR(6,10),2,ac[(int)(R01()*4.99f)]);
            if(i==2) Awning(cx+w/2+0.8f,cz+RR(-d/4,d/4),RR(6,10),3,ac[(int)(R01()*4.99f)]);
            if(j==0) Awning(cx+RR(-w/4,w/4),cz-d/2-0.8f,RR(6,10),0,ac[(int)(R01()*4.99f)]);
            if(j==2) Awning(cx+RR(-w/4,w/4),cz+d/2+0.8f,RR(6,10),1,ac[(int)(R01()*4.99f)]);
        }
        if(h>22&&R01()<0.3f&&(code=='F'||code=='T'||code=='G'||code=='U')) AddDeco(DK_BILL,cx,h,cz,RR(9,13),(float)(i+j),ac[(int)(R01()*4.99f)]);
    }
    if(st.trees) for(int k=0;k<5;k++){ float t=RR(5,165); Tree(x0+t,z0+2.5f,RR(0.8f,1.2f)); Tree(x0+t,z0+167.5f,RR(0.8f,1.2f)); }
    int ns=(code=='O')?10:(code=='M'||code=='F')?7:3;
    for(int k=0;k<ns;k++){
        float t=RR(14,156); int side=(int)(R01()*3.99f), kind=(int)(R01()*3.99f);
        if(side==0) AddStall(x0+t,z0+4.5f,0,kind); else if(side==1) AddStall(x0+t,z0+165.5f,1,kind);
        else if(side==2) AddStall(x0+4.5f,z0+t,2,kind); else AddStall(x0+165.5f,z0+t,3,kind);
    }
}
static void GenStadium(float x0,float z0){
    Quad(x0+85,z0+85,170,170,0.12f,C(150,148,142));
    Quad(x0+85,z0+85,104,74,0.16f,C(60,130,66)); Quad(x0+85,z0+85,2,74,0.18f,C(235,235,235));
    Color wl=C(188,184,174);
    for(int s=0;s<2;s++){ float zz=z0+(s?146:24);
        Blk(x0+45,zz,56,10,13,OB_SOLID,wl,0); Blk(x0+125,zz,56,10,13,OB_SOLID,wl,0); }
    for(int s=0;s<2;s++){ float xx=x0+(s?148:22);
        Blk(xx,z0+52,10,40,13,OB_SOLID,wl,0); Blk(xx,z0+118,10,40,13,OB_SOLID,wl,0); }
    AddDeco(DK_FLOOD,x0+30,0,z0+30,0,0,C(255,240,200)); AddDeco(DK_FLOOD,x0+140,0,z0+30,0,0,C(255,240,200));
    AddDeco(DK_FLOOD,x0+30,0,z0+140,0,0,C(255,240,200)); AddDeco(DK_FLOOD,x0+140,0,z0+140,0,0,C(255,240,200));
    for(int k=0;k<10;k++) Tree(x0+RR(5,165),z0+RR(5,12),1.0f);
}
static void GenPark(float x0,float z0,char code){
    Quad(x0+85,z0+85,170,170,0.12f,C(62,112,62));
    Quad(x0+85,z0+85,6,170,0.14f,C(176,166,140)); Quad(x0+85,z0+85,170,6,0.14f,C(176,166,140));
    int n=(code=='R')?36:20;
    for(int k=0;k<n;k++){ float x=RR(8,162),z=RR(8,162); if(fabsf(x-85)<8||fabsf(z-85)<8) continue; Tree(x0+x,z0+z,RR(1.0f,1.6f)); }
    if(code=='R'){ Quad(x0+45,z0+125,42,30,0.15f,C(36,92,130)); AddObs(x0+24,0,z0+110,x0+66,2,z0+140,OB_WATER,C(0,0,0),0); }
    if(code=='N'){
        Quad(x0+85,z0+85,34,26,0.2f,C(150,45,45));
        Blk(x0+85,z0+85,12,6,12.0f,OB_SOLID,C(215,205,190),8);
        AddDeco(DK_MINAR,x0+85,0.2f,z0+85,1,0,C(215,205,190));
    }
}
static void GenParliament(float x0,float z0){
    Quad(x0+85,z0+85,170,170,0.12f,C(70,120,66));
    Color con=C(196,190,178);
    Blk(x0+85,z0+70,64,64,26,OB_SOLID,con,6);
    Blk(x0+85,z0+28,26,22,16,OB_SOLID,con,6); Blk(x0+85,z0+112,26,22,16,OB_SOLID,con,6);
    Blk(x0+28,z0+70,22,26,16,OB_SOLID,con,6); Blk(x0+142,z0+70,22,26,16,OB_SOLID,con,6);
    Quad(x0+85,z0+150,120,26,0.16f,C(36,92,130)); AddObs(x0+25,0,z0+137,x0+145,2,z0+163,OB_WATER,C(0,0,0),0);
    for(int k=0;k<14;k++) Tree(x0+RR(8,162),z0+RR(8,40),1.1f);
}
static void GenFort(float x0,float z0){
    Quad(x0+85,z0+85,170,170,0.12f,C(76,122,70));
    Color wall=C(168,98,78);
    Blk(x0+85,z0+14,150,3,7,OB_WALL,wall,0);
    Blk(x0+14,z0+85,3,150,7,OB_WALL,wall,0); Blk(x0+156,z0+85,3,150,7,OB_WALL,wall,0);
    Blk(x0+36,z0+156,52,3,7,OB_WALL,wall,0); Blk(x0+134,z0+156,52,3,7,OB_WALL,wall,0);
    Blk(x0+14,z0+14,8,8,10,OB_SOLID,wall,0); Blk(x0+156,z0+14,8,8,10,OB_SOLID,wall,0);
    Blk(x0+14,z0+156,8,8,10,OB_SOLID,wall,0); Blk(x0+156,z0+156,8,8,10,OB_SOLID,wall,0);
    Blk(x0+85,z0+80,30,30,9,OB_SOLID,C(200,170,150),0); AddDeco(DK_DOME,x0+85,9,z0+80,11,0,C(222,206,186));
    Blk(x0+45,z0+55,22,12,8,OB_SOLID,C(200,185,160),0);
    for(int k=0;k<3;k++) AddDeco(DK_DOME,x0+38+k*7,8,z0+55,3.6f,0,C(225,225,215));
    for(int k=0;k<10;k++) Tree(x0+RR(25,145),z0+RR(25,145),1.1f);
}
static void GenPalace(float x0,float z0){
    Quad(x0+85,z0+85,170,170,0.12f,C(80,126,72));
    Blk(x0+85,z0+60,92,26,14,OB_SOLID,C(232,150,165),0); AddDeco(DK_DOME,x0+85,14,z0+60,8.5f,0,C(236,220,210));
    Blk(x0+85,z0+78,22,6,1.3f,OB_SOLID,C(215,205,190),0);
    for(int k=0;k<12;k++) Tree(x0+RR(8,162),z0+RR(100,162),1.2f);
}
static void GenWater(float x0,float z0,bool dock){
    Quad(x0+85,z0+85,200,200,0.05f,C(34,86,128));
    if(dock){
        Quad(x0+85,z0+50,170,100,0.12f,C(146,144,138));
        Blk(x0+45,z0+40,50,24,9,OB_BUILD,C(210,190,160),0); Blk(x0+125,z0+40,50,24,9,OB_BUILD,C(180,200,190),0);
        AddObs(x0,0,z0+105,x0+170,2,z0+200,OB_WATER,C(0,0,0),0);
        for(int k=0;k<6;k++) AddDeco(DK_BOAT,x0+RR(10,160),0.1f,z0+RR(110,190),RR(0,3),0,C(120+(int)RR(0,60),70,40));
    } else {
        AddObs(x0-15,0,z0,x0+185,2,z0+200,OB_WATER,C(0,0,0),0);
        for(int k=0;k<5;k++) AddDeco(DK_BOAT,x0+RR(0,170),0.1f,z0+RR(10,190),RR(0,3),0,C(120+(int)RR(0,60),70,40));
    }
}
static void GenLake(float x0,float z0){
    Quad(x0+85,z0+85,170,170,0.12f,C(82,126,72));
    Quad(x0+85,z0+85,140,140,0.16f,C(36,92,130));
    AddObs(x0+15,0,z0+15,x0+155,2,z0+155,OB_WATER,C(0,0,0),0);
    for(int k=0;k<14;k++){ float t=RR(8,162); Tree(x0+t,z0+RR(2,10),1.1f); Tree(x0+t,z0+RR(160,168),1.1f); }
}

/* ---------- enemy base ---------- */
static void GenBase(float x0,float z0){
    float cx=x0+85,cz=z0+85;
    BASE.x0=x0; BASE.z0=z0; BASE.center=V3(cx,0,cz);
    Quad(cx,cz,170,170,0.12f,C(112,110,102));
    Color wall=C(128,124,116);
    float a=x0+12,b=x0+158,c0=z0+12,c1=z0+158,gap=7;
    /* four walls with centre gates */
    float seg=(b-a)/2-gap, mid1=a+seg/2, mid2=b-seg/2;
    Blk(mid1,c0,seg,2,3.4f,OB_WALL,wall,0); Blk(mid2,c0,seg,2,3.4f,OB_WALL,wall,0);
    Blk(mid1,c1,seg,2,3.4f,OB_WALL,wall,0); Blk(mid2,c1,seg,2,3.4f,OB_WALL,wall,0);
    float m1=c0+seg/2,m2=c1-seg/2;
    Blk(a,m1,2,seg,3.4f,OB_WALL,wall,0); Blk(a,m2,2,seg,3.4f,OB_WALL,wall,0);
    Blk(b,m1,2,seg,3.4f,OB_WALL,wall,0); Blk(b,m2,2,seg,3.4f,OB_WALL,wall,0);
    Color tw=C(112,108,100);
    Blk(x0+17,z0+17,7,7,9,OB_SOLID,tw,9); Blk(x0+153,z0+17,7,7,9,OB_SOLID,tw,9);
    Blk(x0+17,z0+153,7,7,9,OB_SOLID,tw,9); Blk(x0+153,z0+153,7,7,9,OB_SOLID,tw,9);
    Blk(cx,z0+38,36,18,8,OB_SOLID,C(98,104,94),4);                      /* command building */
    Color cont[4]={C(150,50,40),C(40,90,140),C(60,110,70),C(190,150,50)};
    for(int i=0;i<3;i++){
        Blk(x0+34+i*14,z0+78,12,2.6f,2.6f,OB_SOLID,cont[i],1);
        AddObs(x0+28+i*14,2.6f,z0+76.7f,x0+40+i*14,5.2f,z0+79.3f,OB_SOLID,cont[(i+1)%4],1);
        Blk(x0+136-i*14,z0+104,12,2.6f,2.6f,OB_SOLID,cont[(i+2)%4],1);
    }
    Blk(x0+30,z0+120,2.6f,12,2.6f,OB_SOLID,cont[3],1); Blk(x0+140,z0+66,2.6f,12,2.6f,OB_SOLID,cont[0],1);
    Color sand=C(160,140,100);
    Blk(cx,z0+174-2,12,1.2f,1.1f,OB_SOLID,sand,2); Blk(cx,z0-4+2,12,1.2f,1.1f,OB_SOLID,sand,2);
    Blk(x0-4+2,cz,1.2f,12,1.1f,OB_SOLID,sand,2); Blk(x0+174-2,cz,1.2f,12,1.1f,OB_SOLID,sand,2);
    /* weapons cache + sandbags */
    BASE.cache=V3(cx,1.3f,cz+14);
    AddObs(cx-3,0,cz+14-3,cx+3,2.6f,cz+14+3,OB_CACHE,C(60,70,50),0);
    Blk(cx-6.5f,cz+14,1.2f,9,1.1f,OB_SOLID,sand,2); Blk(cx+6.5f,cz+14,1.2f,9,1.1f,OB_SOLID,sand,2);
    for(int k=0;k<8;k++){ float an=k*0.785f; AddBarrel(cx+cosf(an)*10.5f,cz+14+sinf(an)*10.5f); }
    for(int k=0;k<4;k++) AddBarrel(x0+120+(k%2)*1.6f,z0+120+(k/2)*1.6f);
    AddBarrel(x0+60,z0+100); AddBarrel(x0+62,z0+101);
    AddDeco(DK_TENT,x0+50,0,z0+125,0,0,C(80,95,60)); AddDeco(DK_TENT,x0+120,0,z0+130,0,0,C(80,95,60)); AddDeco(DK_TENT,x0+75,0,z0+140,0,0,C(110,100,70));
    AddDeco(DK_FLAG,cx,0,cz-6,0,0,C(120,20,25));
    AddDeco(DK_FLOOD,x0+30,0,z0+30,0,0,C(255,240,200)); AddDeco(DK_FLOOD,x0+140,0,z0+30,0,0,C(255,240,200));
    AddDeco(DK_FLOOD,x0+30,0,z0+140,0,0,C(255,240,200)); AddDeco(DK_FLOOD,x0+140,0,z0+140,0,0,C(255,240,200));
}

/* ---------- traffic, lamps, perimeter ---------- */
static void AddVehicleT(float x,float z,bool alongZ,int type){
    if(type<0){
        float roll=R01(); char code=CodeAt(x,z);
        if(code=='O') type=(roll<0.6f)?0:(roll<0.8f)?1:(roll<0.9f)?2:3;
        else if(code=='G'||code=='T'||code=='U') type=(roll<0.15f)?0:(roll<0.3f)?1:(roll<0.85f)?2:3;
        else type=(roll<0.3f)?0:(roll<0.5f)?1:(roll<0.8f)?2:3;
    }
    static const float W[4]={1.4f,1.6f,1.9f,2.6f}, L[4]={2.7f,2.9f,4.4f,10.5f}, H[4]={1.9f,1.8f,1.5f,3.3f};
    Color cols[6]={C(210,50,45),C(40,110,190),C(240,200,50),C(60,160,90),C(230,230,230),C(150,60,160)};
    Color col=cols[(int)(R01()*5.99f)]; if(type==1) col=C(40,150,80); if(type==3) col=C(210,70,40);
    float w=W[type],l=L[type]; float dx=alongZ?w:l, dz=alongZ?l:w;
    AddObs(x-dx/2,0,z-dz/2,x+dx/2,H[type],z+dz/2,OB_VEH,col,type*2+(alongZ?1:0));
}
static void GenRoadProps(void){
    for(int k=1;k<BLOCKS;k++) for(float t=30;t<WORLD_SZ-30;t+=26){
        float nearI=fabsf(t-200*roundf(t/200)); if(nearI<26) continue;
        for(int s=-1;s<=1;s+=2){
            if(R01()<0.07f) AddVehicleT(k*200+s*RR(8.8f,11.0f),t+RR(-4,4),true,-1);
            if(R01()<0.07f) AddVehicleT(t+RR(-4,4),k*200+s*RR(8.8f,11.0f),false,-1);
        }
    }
    for(int k=0;k<=BLOCKS;k++) for(float t=40;t<WORLD_SZ-40;t+=55){
        if(fabsf(t-200*roundf(t/200))<26) continue;
        AddDeco(DK_LAMP,k*200+14.2f,0,t,0,0,C(255,236,170)); AddDeco(DK_LAMP,t,0,k*200-14.2f,0,0,C(255,236,170));
    }
    for(int k=1;k<BLOCKS;k++) for(int j=1;j<BLOCKS;j++) AddDeco(DK_SIGNAL,k*200+17.5f,0,j*200+17.5f,0,0,C(60,62,68));
}
static void GenPerimeter(void){
    Color w=C(120,118,112); float s=WORLD_SZ;
    AddObs(-6,0,-6,s+6,14,0,OB_SOLID,w,3); AddObs(-6,0,s,s+6,14,s+6,OB_SOLID,w,3);
    AddObs(-6,0,0,0,14,s,OB_SOLID,w,3); AddObs(s,0,0,s+6,14,s,OB_SOLID,w,3);
}

/* ---------- mini-map texture ---------- */
static void BuildMiniMap(void){
    float sc=MAP_PX/(WORLD_SZ+MAP_MARGIN*2);
    Image im=GenImageColor(MAP_PX,MAP_PX,C(14,18,26));
#define MR(x0,z0,x1,z1,col) ImageDrawRectangle(&im,(int)(((x0)+MAP_MARGIN)*sc),(int)(((z0)+MAP_MARGIN)*sc),(int)(((x1)-(x0))*sc)+1,(int)(((z1)-(z0))*sc)+1,col)
    MR(0,0,WORLD_SZ,WORLD_SZ,C(42,48,58));
    for(int k=0;k<=BLOCKS;k++){ MR(k*200-15,0,k*200+15,WORLD_SZ,C(96,100,110)); MR(0,k*200-15,WORLD_SZ,k*200+15,C(96,100,110)); }
    for(int i=0;i<nDeco;i++) if(DECO[i].kind==DK_QUAD && DECO[i].a<=200){
        Deco *d=&DECO[i]; if(d->p.y<0.1f) continue;
        MR(d->p.x-d->a/2,d->p.z-d->b/2,d->p.x+d->a/2,d->p.z+d->b/2,ColMul(d->col,(d->col.b>d->col.r+30)?1.0f:0.55f));
    }
    for(int i=0;i<nObs;i++){ Obs *o=&OBS[i]; if(!o->active) continue;
        Color c; if(o->kind==OB_BUILD) c=C(78,88,110); else if(o->kind==OB_SOLID||o->kind==OB_WALL) c=C(66,72,90); else if(o->kind==OB_WATER) c=C(36,92,140); else continue;
        MR(o->mn.x,o->mn.z,o->mx.x,o->mx.z,c); }
    MR(BASE.x0,BASE.z0,BASE.x0+170,BASE.z0+3,C(200,50,50)); MR(BASE.x0,BASE.z0+167,BASE.x0+170,BASE.z0+170,C(200,50,50));
    MR(BASE.x0,BASE.z0,BASE.x0+3,BASE.z0+170,C(200,50,50)); MR(BASE.x0+167,BASE.z0,BASE.x0+170,BASE.z0+170,C(200,50,50));
#undef MR
    if(haveMap) UnloadTexture(mapTex);
    mapTex=LoadTextureFromImage(im); SetTextureFilter(mapTex,TEXTURE_FILTER_BILINEAR); UnloadImage(im); haveMap=true;
}

void WorldInit(void){
    Image im=GenImageColor(128,64,C(222,218,208));
    rs=777;
    for(int f=0;f<2;f++){
        int y0=f*32;
        ImageDrawRectangle(&im,0,y0,128,3,C(160,156,148));
        for(int i=0;i<4;i++){
            int x=6+i*32; bool lit=R01()<0.30f;
            ImageDrawRectangle(&im,x-2,y0+6,24,22,C(112,110,106));
            ImageDrawRectangle(&im,x,y0+8,20,18,lit?C(255,214,128):C(48,70,94));
            if(!lit) ImageDrawRectangle(&im,x,y0+8,20,4,C(78,106,134));
            ImageDrawRectangle(&im,x-3,y0+28,26,3,C(184,180,170));
        }
    }
    texWin=LoadTextureFromImage(im); UnloadImage(im);
    GenTextureMipmaps(&texWin); SetTextureFilter(texWin,TEXTURE_FILTER_TRILINEAR); SetTextureWrap(texWin,TEXTURE_WRAP_REPEAT);
}

void WorldGenerate(int baseR,int baseC){
    nObs=0; nDeco=0; nBarrel=0; nStall=0; rs=20991u+baseR*31+baseC*7;
    BASE.br=baseR; BASE.bc=baseC;
    GenPerimeter();
    for(int r=0;r<BLOCKS;r++) for(int c=0;c<BLOCKS;c++){
        float x0=c*BLOCK_SZ+15, z0=r*BLOCK_SZ+15; char code=DMAP[r][c];
        if(r==baseR&&c==baseC){ GenBase(x0,z0); continue; }
        switch(code){
            case 'R': case 'N': GenPark(x0,z0,code); break;
            case 'J': GenParliament(x0,z0); break; case 'B': GenFort(x0,z0); break; case 'A': GenPalace(x0,z0); break;
            case 'L': case 'H': GenLake(x0,z0); break; case 'C': GenStadium(x0,z0); break; case 'S': GenWater(x0,z0,true); break; case '~': GenWater(x0,z0,false); break;
            default: GenCity(x0,z0,code); break;
        }
    }
    GenRoadProps();
    WaterBuild(); GridBuild();
    NavBuild();
    BuildMiniMap();
}

/* ---------- collision & ray casting ---------- */
bool RayAABB(Vector3 o,Vector3 d,Vector3 mn,Vector3 mx,float *t){
    float tmin=0,tmax=1e9f;
    float oo[3]={o.x,o.y,o.z},dd[3]={d.x,d.y,d.z},a[3]={mn.x,mn.y,mn.z},b[3]={mx.x,mx.y,mx.z};
    for(int i=0;i<3;i++){
        if(fabsf(dd[i])<1e-8f){ if(oo[i]<a[i]||oo[i]>b[i]) return false; }
        else{
            float inv=1.0f/dd[i],t1=(a[i]-oo[i])*inv,t2=(b[i]-oo[i])*inv;
            if(t1>t2){float s=t1;t1=t2;t2=s;}
            if(t1>tmin)tmin=t1; if(t2<tmax)tmax=t2; if(tmin>tmax) return false;
        }
    }
    *t=tmin; return true;
}
float WorldRayT(Vector3 o,Vector3 d,float maxT,int *obsIdx){
    float best=maxT; int bi=-1;
    if(d.y<-1e-5f){ float tg=-o.y/d.y; if(tg>0&&tg<best){best=tg;bi=-2;} }
    for(int i=0;i<nObs;i++){
        const Obs *ob=&OBS[i]; if(!ob->active||ob->kind==OB_WATER) continue;
        float t; if(RayAABB(o,d,ob->mn,ob->mx,&t)&&t<best){best=t;bi=i;}
    }
    if(obsIdx)*obsIdx=bi; return best;
}
#define GC 40.0f
#define GNN ((int)(WORLD_SZ/GC))
#define WN  ((int)(WORLD_SZ/10.0f))
static int gStart[(2400/40)*(2400/40)+2]; static int *gItems=NULL;
static unsigned char waterM[(2400/10)*(2400/10)];
static void GridBuild(void){
    static int cnt[(2400/40)*(2400/40)+2];
    int n=GNN*GNN; memset(cnt,0,sizeof(cnt));
    for(int pass=0;pass<2;pass++){
        if(pass==1){ int tot=0; for(int c=0;c<n;c++){ gStart[c]=tot; tot+=cnt[c]; cnt[c]=gStart[c]; } gStart[n]=tot; free(gItems); gItems=(int*)malloc(sizeof(int)*(tot+1)); }
        for(int i=0;i<nObs;i++){
            int x0=(int)(OBS[i].mn.x/GC),x1=(int)(OBS[i].mx.x/GC),z0=(int)(OBS[i].mn.z/GC),z1=(int)(OBS[i].mx.z/GC);
            if(x0<0)x0=0; if(z0<0)z0=0; if(x1>=GNN)x1=GNN-1; if(z1>=GNN)z1=GNN-1;
            for(int z=z0;z<=z1;z++) for(int x=x0;x<=x1;x++){ int c=z*GNN+x; if(pass==0) cnt[c]++; else gItems[cnt[c]++]=i; }
        }
    }
}
static void WaterBuild(void){
    memset(waterM,0,sizeof(waterM));
    for(int i=0;i<nObs;i++) if(OBS[i].kind==OB_WATER){
        for(int z=0;z<WN;z++) for(int x=0;x<WN;x++){ float cx=x*10+5.0f,cz=z*10+5.0f;
            if(cx>OBS[i].mn.x&&cx<OBS[i].mx.x&&cz>OBS[i].mn.z&&cz<OBS[i].mx.z) waterM[z*WN+x]=1; } }
}
void MoveCollide(Vector3 *p,float r,float *groundOut){
    float g=0;
    p->x=Clampf(p->x,3,WORLD_SZ-3); p->z=Clampf(p->z,3,WORLD_SZ-3);
    int cx0=(int)((p->x-r)/GC),cx1=(int)((p->x+r)/GC),cz0=(int)((p->z-r)/GC),cz1=(int)((p->z+r)/GC);
    if(cx0<0)cx0=0; if(cz0<0)cz0=0; if(cx1>=GNN)cx1=GNN-1; if(cz1>=GNN)cz1=GNN-1;
    for(int pass=0;pass<2;pass++)
      for(int cz=cz0;cz<=cz1;cz++) for(int cx=cx0;cx<=cx1;cx++){
        int c=cz*GNN+cx;
        for(int q=gStart[c];q<gStart[c+1];q++){
            const Obs *o=&OBS[gItems[q]]; if(!o->active) continue;
            if(p->x+r<o->mn.x||p->x-r>o->mx.x||p->z+r<o->mn.z||p->z-r>o->mx.z) continue;
            float top=o->mx.y;
            if(top<=p->y+0.35f){
                if(pass==0&&p->x>o->mn.x&&p->x<o->mx.x&&p->z>o->mn.z&&p->z<o->mx.z&&top>g) g=top;
                continue;
            }
            if(o->mn.y>p->y+1.8f) continue;
            float qx=Clampf(p->x,o->mn.x,o->mx.x),qz=Clampf(p->z,o->mn.z,o->mx.z);
            float dx=p->x-qx,dz=p->z-qz,d2=dx*dx+dz*dz;
            if(d2>=r*r) continue;
            if(d2>1e-8f){ float d=sqrtf(d2); p->x+=dx/d*(r-d); p->z+=dz/d*(r-d); }
            else{
                float pl=p->x-o->mn.x,pr=o->mx.x-p->x,pb=p->z-o->mn.z,pf=o->mx.z-p->z,m=pl;
                int side=0; if(pr<m){m=pr;side=1;} if(pb<m){m=pb;side=2;} if(pf<m){m=pf;side=3;}
                if(side==0)p->x=o->mn.x-r; else if(side==1)p->x=o->mx.x+r; else if(side==2)p->z=o->mn.z-r; else p->z=o->mx.z+r;
            }
        }
    }
    if(groundOut)*groundOut=g;
}
bool WorldFree(float x,float z,float r){
    if(x<r+4||z<r+4||x>WORLD_SZ-r-4||z>WORLD_SZ-r-4) return false;
    int cx0=(int)((x-r)/GC),cx1=(int)((x+r)/GC),cz0=(int)((z-r)/GC),cz1=(int)((z+r)/GC);
    if(cx0<0)cx0=0; if(cz0<0)cz0=0; if(cx1>=GNN)cx1=GNN-1; if(cz1>=GNN)cz1=GNN-1;
    for(int cz=cz0;cz<=cz1;cz++) for(int cx=cx0;cx<=cx1;cx++){ int c=cz*GNN+cx;
        for(int q=gStart[c];q<gStart[c+1];q++){ const Obs *o=&OBS[gItems[q]]; if(!o->active||o->mx.y<0.9f) continue;
            if(x+r>o->mn.x&&x-r<o->mx.x&&z+r>o->mn.z&&z-r<o->mx.z) return false; } }
    return true;
}
bool IsWaterAt(float x,float z){
    int ix=(int)(x/10.0f),iz=(int)(z/10.0f); if(ix<0||iz<0||ix>=WN||iz>=WN) return false; return waterM[iz*WN+ix]!=0;
}

/* ---------- navigation flow field (4 m cells, 180 m radius around the target) ---------- */
#define NAVN 600
#define NAVR 45
static unsigned char navB[NAVN*NAVN];
static unsigned short navD[NAVN*NAVN];
static unsigned int navSeen[NAVN*NAVN];
static unsigned int navStamp=0;
static int navQ[(2*NAVR+1)*(2*NAVR+1)+8];
void NavBuild(void){
    memset(navB,0,sizeof(navB)); memset(navSeen,0,sizeof(navSeen)); navStamp=0;
    for(int i=0;i<nObs;i++){ const Obs *o=&OBS[i]; if(!o->active||o->mx.y<0.9f||o->mn.y>1.5f) continue;
        int x0=(int)((o->mn.x-0.7f)/4),x1=(int)((o->mx.x+0.7f)/4),z0=(int)((o->mn.z-0.7f)/4),z1=(int)((o->mx.z+0.7f)/4);
        if(x0<0)x0=0; if(z0<0)z0=0; if(x1>=NAVN)x1=NAVN-1; if(z1>=NAVN)z1=NAVN-1;
        for(int z=z0;z<=z1;z++) for(int x=x0;x<=x1;x++) navB[z*NAVN+x]=1;
    }
}
void NavFlow(Vector3 t){
    navStamp++;
    int cx=(int)(t.x/4),cz=(int)(t.z/4); cx=cx<0?0:(cx>=NAVN?NAVN-1:cx); cz=cz<0?0:(cz>=NAVN?NAVN-1:cz);
    int qh=0,qt=0; int s=cz*NAVN+cx; navQ[qt++]=s; navSeen[s]=navStamp; navD[s]=0;
    static const int dx[8]={1,-1,0,0,1,1,-1,-1}, dz[8]={0,0,1,-1,1,-1,1,-1};
    while(qh<qt){
        int c=navQ[qh++],x=c%NAVN,z=c/NAVN; unsigned short dd=navD[c];
        for(int k=0;k<8;k++){
            int nx=x+dx[k],nz=z+dz[k];
            if(nx<0||nz<0||nx>=NAVN||nz>=NAVN||abs(nx-cx)>NAVR||abs(nz-cz)>NAVR) continue;
            int ni=nz*NAVN+nx; if(navSeen[ni]==navStamp||navB[ni]) continue;
            if(k>=4&&(navB[z*NAVN+nx]||navB[nz*NAVN+x])) continue;
            navSeen[ni]=navStamp; navD[ni]=dd+1; navQ[qt++]=ni;
        }
    }
}
bool NavDir(Vector3 from,Vector3 *out){
    int x=(int)(from.x/4),z=(int)(from.z/4); if(x<0||z<0||x>=NAVN||z>=NAVN) return false;
    int c=z*NAVN+x; int best=-1; unsigned int bd=(navSeen[c]==navStamp)?navD[c]:65535;
    static const int dx[8]={1,-1,0,0,1,1,-1,-1}, dz[8]={0,0,1,-1,1,-1,1,-1};
    for(int k=0;k<8;k++){
        int nx=x+dx[k],nz=z+dz[k]; if(nx<0||nz<0||nx>=NAVN||nz>=NAVN) continue;
        int ni=nz*NAVN+nx; if(navSeen[ni]!=navStamp) continue;
        if(navD[ni]<bd){ bd=navD[ni]; best=ni; }
    }
    if(best<0) return false;
    float tx=(best%NAVN)*4+2.0f, tz=(best/NAVN)*4+2.0f;
    *out=V3Norm(V3(tx-from.x,0,tz-from.z)); return true;
}

/* ---------- rendering ---------- */
void Begin3D(Camera3D cam){
    rlDrawRenderBatchActive();
    rlMatrixMode(RL_PROJECTION); rlPushMatrix(); rlLoadIdentity();
    float aspect=(float)GetScreenWidth()/(float)GetScreenHeight();
    double nearP=0.15,farP=900.0, top=nearP*tan(cam.fovy*0.5*DEG2RAD), right=top*aspect;
    rlFrustum(-right,right,-top,top,nearP,farP);
    rlMatrixMode(RL_MODELVIEW); rlLoadIdentity();
    Matrix v=MatrixLookAt(cam.position,cam.target,cam.up); rlMultMatrixf(MatrixToFloat(v));
    rlEnableDepthTest();
}
void End3D(void){
    rlDrawRenderBatchActive();
    rlMatrixMode(RL_PROJECTION); rlPopMatrix();
    rlMatrixMode(RL_MODELVIEW); rlLoadIdentity();
    rlDisableDepthTest();
}
static const Color FOGC={222,172,130,255};
static Color Fog(Color c,float d,float R){ float t=Clampf((d-R*0.4f)/(R*0.6f),0,1); return ColMix(c,FOGC,t*0.92f); }

static void Facades(const Obs *o,Color tint){
    float x0=o->mn.x,x1=o->mx.x,z0=o->mn.z,z1=o->mx.z,h=o->mx.y;
    float u0=(x1-x0)/16.0f,u1=(z1-z0)/16.0f,v=h/7.0f;
    rlCheckRenderBatchLimit(16);
    rlSetTexture(texWin.id); rlBegin(RL_QUADS);
#define F(sh) rlColor4ub((unsigned char)(tint.r*(sh)),(unsigned char)(tint.g*(sh)),(unsigned char)(tint.b*(sh)),255)
    F(0.86f); rlTexCoord2f(0,v);rlVertex3f(x0,0,z1); rlTexCoord2f(u0,v);rlVertex3f(x1,0,z1); rlTexCoord2f(u0,0);rlVertex3f(x1,h,z1); rlTexCoord2f(0,0);rlVertex3f(x0,h,z1);
    F(0.58f); rlTexCoord2f(0,v);rlVertex3f(x1,0,z0); rlTexCoord2f(u0,v);rlVertex3f(x0,0,z0); rlTexCoord2f(u0,0);rlVertex3f(x0,h,z0); rlTexCoord2f(0,0);rlVertex3f(x1,h,z0);
    F(0.72f); rlTexCoord2f(0,v);rlVertex3f(x1,0,z1); rlTexCoord2f(u1,v);rlVertex3f(x1,0,z0); rlTexCoord2f(u1,0);rlVertex3f(x1,h,z0); rlTexCoord2f(0,0);rlVertex3f(x1,h,z1);
    F(0.64f); rlTexCoord2f(0,v);rlVertex3f(x0,0,z0); rlTexCoord2f(u1,v);rlVertex3f(x0,0,z1); rlTexCoord2f(u1,0);rlVertex3f(x0,h,z1); rlTexCoord2f(0,0);rlVertex3f(x0,h,z0);
#undef F
    rlEnd();
}
void DrawVehicleAt(int type,Vector3 pos,float yaw,Color col,bool braking){
    Color dark=C(28,28,32),glass=C(70,95,120);
    rlPushMatrix(); rlTranslatef(pos.x,pos.y,pos.z); rlRotatef(yaw*RAD2DEG,0,1,0);
    if(type==2){
        BoxC(V3(0,0.55f,0),V3(1.9f,0.7f,4.4f),col); BoxC(V3(0,1.12f,-0.2f),V3(1.65f,0.55f,2.3f),glass);
        for(int sx=-1;sx<=1;sx+=2) for(int sz=-1;sz<=1;sz+=2) BoxC(V3(sx*0.9f,0.3f,sz*1.4f),V3(0.25f,0.6f,0.6f),dark);
    } else if(type==0){
        BoxC(V3(0,0.45f,0),V3(0.9f,0.22f,2.5f),col); BoxC(V3(0,0.75f,-0.4f),V3(0.85f,0.3f,0.9f),C(90,60,40));
        BoxC(V3(0,1.75f,-0.45f),V3(1.35f,0.14f,1.5f),ColMix(col,C(250,220,60),0.5f));
        BoxC(V3(-0.55f,1.1f,-1.1f),V3(0.05f,1.3f,0.05f),dark); BoxC(V3(0.55f,1.1f,-1.1f),V3(0.05f,1.3f,0.05f),dark);
        BoxC(V3(0,0.35f,1.05f),V3(0.1f,0.7f,0.7f),dark); BoxC(V3(-0.6f,0.35f,-0.7f),V3(0.1f,0.7f,0.7f),dark); BoxC(V3(0.6f,0.35f,-0.7f),V3(0.1f,0.7f,0.7f),dark);
    } else if(type==1){
        BoxC(V3(0,0.85f,0),V3(1.5f,1.2f,2.6f),col); BoxC(V3(0,1.55f,-0.1f),V3(1.45f,0.2f,2.3f),dark); BoxC(V3(0,1.0f,1.28f),V3(1.2f,0.5f,0.06f),glass);
        BoxC(V3(0,0.3f,1.0f),V3(0.12f,0.6f,0.6f),dark); BoxC(V3(-0.7f,0.3f,-0.9f),V3(0.2f,0.6f,0.6f),dark); BoxC(V3(0.7f,0.3f,-0.9f),V3(0.2f,0.6f,0.6f),dark);
    } else {
        BoxC(V3(0,1.55f,0),V3(2.6f,2.4f,10.4f),col); BoxC(V3(0,2.0f,0),V3(2.62f,0.9f,9.4f),glass);
        BoxC(V3(0,0.95f,0),V3(2.62f,0.3f,10.0f),C(250,230,60)); BoxC(V3(0,3.3f,0),V3(2.4f,0.12f,10.0f),ColMul(col,0.7f));
        for(int sx=-1;sx<=1;sx+=2) for(int sz=-1;sz<=1;sz+=2) BoxC(V3(sx*1.2f,0.5f,sz*3.4f),V3(0.3f,1.0f,1.0f),dark);
    }
    static const float HL[4]={1.35f,1.45f,2.2f,5.2f}, LY[4]={0.5f,0.9f,0.65f,1.0f}, LX[4]={0.32f,0.5f,0.65f,0.95f};
    Color tail=braking?(Color){255,30,30,255}:(Color){110,12,12,255};
    for(int s=-1;s<=1;s+=2){
        BoxC(V3(LX[type]*s,LY[type],HL[type]+0.02f),V3(0.22f,0.16f,0.05f),(Color){255,244,200,255});
        BoxC(V3(LX[type]*s,LY[type],-HL[type]-0.02f),V3(0.22f,0.14f,0.05f),tail);
    }
    rlPopMatrix();
}
static void DrawVehicle(const Obs *o,Color col){
    int type=o->style>>1; bool alongZ=o->style&1;
    DrawVehicleAt(type,V3((o->mn.x+o->mx.x)/2,0,(o->mn.z+o->mx.z)/2),alongZ?0.0f:PI/2,col,false);
}
static void DrawStall(const Obs *o,Color body){
    int kind=(o->style-100)/4, face=(o->style-100)%4; static const float yw[4]={PI,0,-PI/2,PI/2};
    static const Color good[4][3]={{{190,190,196,255},{240,240,235,255},{150,90,50,255}},{{240,140,30,255},{200,40,40,255},{90,170,60,255}},
        {{250,230,200,255},{230,160,50,255},{190,190,196,255}},{{235,235,225,255},{70,110,190,255},{200,60,60,255}}};
    rlPushMatrix(); rlTranslatef((o->mn.x+o->mx.x)/2,0,(o->mn.z+o->mx.z)/2); rlRotatef(yw[face]*RAD2DEG,0,1,0);
    BoxC(V3(0,1.15f,-0.95f),V3(3.0f,2.3f,0.3f),ColMul(body,0.75f)); BoxC(V3(0,0.55f,0.15f),V3(3.0f,1.1f,1.9f),body);
    BoxC(V3(0,2.35f,0.0f),V3(3.5f,0.12f,2.5f),ColMix(body,WHITE,0.35f)); BoxC(V3(-0.9f,2.38f,0.0f),V3(0.5f,0.14f,2.52f),WHITE); BoxC(V3(0.9f,2.38f,0.0f),V3(0.5f,0.14f,2.52f),WHITE);
    BoxC(V3(-1.55f,1.2f,1.1f),V3(0.08f,2.4f,0.08f),C(80,60,40)); BoxC(V3(1.55f,1.2f,1.1f),V3(0.08f,2.4f,0.08f),C(80,60,40));
    for(int k=0;k<5;k++) BoxC(V3(-1.1f+k*0.55f,1.25f,0.5f),V3(0.4f,0.28f,0.4f),good[kind][k%3]);
    rlPopMatrix();
}
static void DrawCache(const Obs *o,float t){
    Vector3 c=V3((o->mn.x+o->mx.x)/2,0,(o->mn.z+o->mx.z)/2);
    BoxC(V3(c.x,0.9f,c.z),V3(5.6f,1.8f,5.6f),C(62,72,52));
    BoxC(V3(c.x-1.3f,2.2f,c.z-1.0f),V3(2.4f,0.9f,2.4f),C(78,70,48)); BoxC(V3(c.x+1.4f,2.2f,c.z+1.0f),V3(2.2f,0.9f,2.2f),C(58,66,76));
    float p=0.6f+0.4f*sinf(t*5.0f);
    BoxC(V3(c.x,1.95f,c.z+2.9f),V3(1.6f,0.5f,0.1f),(Color){(unsigned char)(255*p),(unsigned char)(90*p),20,255});
    BoxC(V3(c.x,1.95f,c.z-2.9f),V3(1.6f,0.5f,0.1f),(Color){(unsigned char)(255*p),(unsigned char)(90*p),20,255});
}
static void DrawDeco(const Deco *d,float dist,float R,float t){
    Vector3 p=d->p;
    switch(d->kind){
        case DK_TREE: { float s=d->a; BoxC(V3(p.x,1.6f*s,p.z),V3(0.5f*s,3.2f*s,0.5f*s),C(86,60,38));
            BoxC(V3(p.x,4.0f*s,p.z),V3(4.2f*s,3.2f*s,4.2f*s),Fog(d->col,dist,R)); BoxC(V3(p.x,6.0f*s,p.z),V3(2.8f*s,2.4f*s,2.8f*s),Fog(ColMul(d->col,1.15f),dist,R)); } break;
        case DK_LAMP: BoxC(V3(p.x,3.4f,p.z),V3(0.18f,6.8f,0.18f),C(70,72,78)); BoxC(V3(p.x,6.9f,p.z),V3(0.9f,0.22f,0.45f),d->col); break;
        case DK_BOAT: { float yw=d->a; rlPushMatrix(); rlTranslatef(p.x,p.y,p.z); rlRotatef(yw*57.3f,0,1,0);
            BoxC(V3(0,0.35f,0),V3(2.6f,0.7f,9.0f),d->col); BoxC(V3(0,1.2f,-1.0f),V3(2.0f,1.1f,3.5f),C(200,190,170)); BoxC(V3(0,1.9f,-1.0f),V3(2.2f,0.15f,3.8f),C(60,90,150)); rlPopMatrix(); } break;
        case DK_DOME: DrawSphereEx(p,d->a,12,12,Fog(d->col,dist,R)); break;
        case DK_FLAG: BoxC(V3(p.x,5,p.z),V3(0.2f,10,0.2f),C(180,180,180)); BoxC(V3(p.x+1.6f,9,p.z),V3(3.0f,1.8f,0.08f),d->col); break;
        case DK_TENT: BoxC(V3(p.x,1.2f,p.z),V3(5,2.4f,7),d->col); BoxC(V3(p.x,2.6f,p.z),V3(3,0.8f,7),ColMul(d->col,0.9f)); break;
        case DK_FLOOD: BoxC(V3(p.x,3.5f,p.z),V3(0.2f,7,0.2f),C(70,72,78)); BoxC(V3(p.x,7.2f,p.z),V3(1.2f,0.7f,0.5f),d->col); break;
        case DK_AWNING: { int f=(int)d->b; float L=d->a; bool ns=(f<2); Vector3 sz=ns?V3(L,0.18f,1.8f):V3(1.8f,0.18f,L); Color c=Fog(d->col,dist,R);
            BoxC(p,sz,c); for(int s=-1;s<=1;s+=2){ Vector3 q=ns?V3(p.x+s*L*0.25f,p.y+0.02f,p.z):V3(p.x,p.y+0.02f,p.z+s*L*0.25f); BoxC(q,ns?V3(L*0.12f,0.2f,1.82f):V3(1.82f,0.2f,L*0.12f),WHITE); } } break;
        case DK_BILL: { bool sw=((int)d->b)%2; BoxC(V3(p.x,p.y+3,p.z),V3(0.5f,6,0.5f),C(70,72,78)); BoxC(V3(p.x,p.y+7.5f,p.z),sw?V3(0.5f,4.2f,d->a):V3(d->a,4.2f,0.5f),Fog(d->col,dist,R)); } break;
        case DK_POLE: BoxC(V3(p.x,d->a/2,p.z),V3(1.3f,d->a,1.3f),Fog(d->col,dist,R)); BoxC(V3(p.x,d->a+0.8f,p.z),V3(1.8f,1.6f,1.8f),Fog(ColMul(d->col,0.9f),dist,R)); BoxC(V3(p.x,d->a+2.0f,p.z),V3(0.5f,1.2f,0.5f),C(70,150,110)); break;
        case DK_SIGNAL: { Color a=SignalState(0)?(Color){60,230,90,255}:(Color){240,50,50,255}, b2=SignalState(1)?(Color){60,230,90,255}:(Color){240,50,50,255};
            BoxC(V3(p.x,3.0f,p.z),V3(0.2f,6,0.2f),d->col); BoxC(V3(p.x-0.45f,5.8f,p.z),V3(0.55f,1.1f,0.55f),C(30,30,34)); BoxC(V3(p.x-0.45f,6.0f,p.z),V3(0.35f,0.35f,0.58f),a);
            BoxC(V3(p.x,5.8f,p.z-0.45f),V3(0.55f,1.1f,0.55f),C(30,30,34)); BoxC(V3(p.x,6.0f,p.z-0.45f),V3(0.58f,0.35f,0.35f),b2); } break;
        case DK_MINAR: for(int i=-2;i<=2;i++){ float h=(abs(i)==2)?7.5f:(abs(i)==1)?9.5f:12.0f; BoxC(V3(p.x+i*2.2f,h/2+0.2f,p.z+((i==0)?0.0f:1.4f)),V3(0.6f,h,0.6f),d->col); } break;
    }
    (void)t;
}
void WorldDraw(Vector3 cam,Vector3 fwd,int dl,float t){
    float R=(dl==0)?220:(dl==1)?340:500;
    DrawFlat(WORLD_SZ/2,-0.02f,WORLD_SZ/2,WORLD_SZ+160,WORLD_SZ+160,C(92,84,72));
    Color road=C(54,56,60);
    for(int k=0;k<=BLOCKS;k++){
        DrawFlat(k*200,0.03f,WORLD_SZ/2,ROAD_W,WORLD_SZ,road); DrawFlat(WORLD_SZ/2,0.03f,k*200,WORLD_SZ,ROAD_W,road);
        if(fabsf(cam.x-k*200)<R) for(float z=floorf((cam.z-R)/12)*12;z<cam.z+R;z+=12){ if(z<0||z>WORLD_SZ||fabsf(z-200*roundf(z/200))<17) continue; DrawFlat(k*200,0.05f,z,0.35f,5.5f,C(230,200,80)); }
        if(fabsf(cam.z-k*200)<R) for(float x=floorf((cam.x-R)/12)*12;x<cam.x+R;x+=12){ if(x<0||x>WORLD_SZ||fabsf(x-200*roundf(x/200))<17) continue; DrawFlat(x,0.05f,k*200,5.5f,0.35f,C(230,200,80)); }
    }
    for(int i=0;i<nDeco;i++){ const Deco *d=&DECO[i]; if(d->kind!=DK_QUAD) continue;
        float dx=d->p.x-cam.x,dz=d->p.z-cam.z; if(dx*dx+dz*dz>(R+130)*(R+130)) continue;
        DrawFlat(d->p.x,d->p.y,d->p.z,d->a,d->b,d->col); }
    static int vis[MAX_OBS]; int nv=0; float R2=R*R;
    for(int i=0;i<nObs;i++){ const Obs *o=&OBS[i]; if(!o->active||o->kind==OB_WATER) continue;
        float cx=(o->mn.x+o->mx.x)*0.5f-cam.x,cz=(o->mn.z+o->mx.z)*0.5f-cam.z,d2=cx*cx+cz*cz;
        if(d2>R2) continue; if(cx*fwd.x+cz*fwd.z<-45&&d2>3600) continue; vis[nv++]=i; }
    for(int k=0;k<nv;k++){ const Obs *o=&OBS[vis[k]]; if(o->kind!=OB_BUILD) continue;
        float cx=(o->mn.x+o->mx.x)*0.5f-cam.x,cz=(o->mn.z+o->mx.z)*0.5f-cam.z; Facades(o,Fog(o->col,sqrtf(cx*cx+cz*cz),R)); }
    for(int k=0;k<nv;k++){ const Obs *o=&OBS[vis[k]];
        float cx=(o->mn.x+o->mx.x)*0.5f-cam.x,cz=(o->mn.z+o->mx.z)*0.5f-cam.z,dist=sqrtf(cx*cx+cz*cz);
        Vector3 c=V3((o->mn.x+o->mx.x)*0.5f,(o->mn.y+o->mx.y)*0.5f,(o->mn.z+o->mx.z)*0.5f), s=V3Sub(o->mx,o->mn);
        switch(o->kind){
            case OB_BUILD: BoxC(V3(c.x,o->mx.y+0.15f,c.z),V3(s.x,0.3f,s.z),Fog(C(96,96,100),dist,R));
                if(o->style&1) BoxC(V3(c.x+s.x*0.2f,o->mx.y+1.8f,c.z-s.z*0.2f),V3(4,3,4),Fog(C(70,110,150),dist,R)); break;
            case OB_SOLID: case OB_WALL:
                if(o->style>=100&&o->style<120){ DrawStall(o,Fog(o->col,dist,R)); break; }
                BoxC(c,s,Fog(o->col,dist,R));
                if(o->style==20){ BoxC(V3(c.x,7.0f,c.z),V3(s.x+0.3f,1.0f,s.z+0.3f),Fog(C(70,150,110),dist,R)); BoxC(V3(c.x,2.2f,c.z+s.z/2+0.05f),V3(4.0f,4.4f,0.2f),C(60,45,35)); } if(o->style==9) BoxC(V3(c.x,o->mx.y+1.2f,c.z),V3(s.x+1.2f,0.3f,s.z+1.2f),Fog(C(80,80,76),dist,R)); break;
            case OB_VEH: DrawVehicle(o,Fog(o->col,dist,R)); break;
            case OB_CACHE: if(M.cacheAlive) DrawCache(o,t); break;
            default: break;
        }
    }
    for(int i=0;i<nBarrel;i++){ const Barrel *b=&BARREL[i]; if(!b->alive) continue;
        float dx=b->pos.x-cam.x,dz=b->pos.z-cam.z; if(dx*dx+dz*dz>R2*0.4f) continue;
        BoxC(V3(b->pos.x,0.55f,b->pos.z),V3(0.7f,1.1f,0.7f),C(180,42,32)); BoxC(V3(b->pos.x,0.55f,b->pos.z),V3(0.74f,0.12f,0.74f),C(240,200,50)); }
    for(int i=0;i<nDeco;i++){ const Deco *d=&DECO[i]; if(d->kind==DK_QUAD) continue;
        float dx=d->p.x-cam.x,dz=d->p.z-cam.z,d2=dx*dx+dz*dz; if(d2>R2) continue; if(dx*fwd.x+dz*fwd.z<-25&&d2>1600) continue;
        DrawDeco(d,sqrtf(d2),R,t); }
}
