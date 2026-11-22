/* Aluminum - a self-contained x86 kernel. No Linux code, no external runtime. */
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
typedef unsigned long long u64;

#include "font8.h"
#include "cjk.h"

static inline void outb(u16 p, u8 v){ __asm__ volatile("outb %0,%1"::"a"(v),"Nd"(p)); }
static inline u8 inb(u16 p){ u8 r; __asm__ volatile("inb %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline void outw(u16 p, u16 v){ __asm__ volatile("outw %0,%1"::"a"(v),"Nd"(p)); }
static inline u16 inw(u16 p){ u16 r; __asm__ volatile("inw %1,%0":"=a"(r):"Nd"(p)); return r; }
static inline void io_wait(void){ outb(0x80,0); }

/* ---- serial ---- */
static void ser_init(void){ outb(0x3F8+1,0); outb(0x3F8+3,0x80); outb(0x3F8+0,1); outb(0x3F8+1,0); outb(0x3F8+3,3); outb(0x3F8+2,0xC7); outb(0x3F8+4,0x0B); }
static void ser_putc(char c){ while(!(inb(0x3F8+5)&0x20)); outb(0x3F8,c); }
static void ser_puts(const char*s){ while(*s){ if(*s=='\n') ser_putc('\r'); ser_putc(*s++);} }
static void ser_hex(u64 v){ char b[17]; b[16]=0; for(int i=15;i>=0;i--){ int d=v&15; b[i]=d<10?'0'+d:'a'+d-10; v>>=4;} ser_puts(b); }
static void ser_dec(u64 v){ char b[24]; int i=23; b[i]=0; if(!v){ser_putc('0');return;} while(v&&i){b[--i]='0'+v%10; v/=10;} ser_puts(b+i); }

/* ---- text output sink: fb or vga text ---- */
static u8 *fb; static u32 W,H,PITCH,BPP;
static int have_fb;
static u32 TXT=0xE6EDF3, DIM=0x8B98A5, ACC=0x2DD4BF, BG=0x0B0E11, PANEL=0x141A20, EDGE=0x243039, WARN=0xF0B429, RED=0xFF5C5C;

static void put(int x,int y,u32 c){
    if(!have_fb) return;
    if(x<0||y<0||x>=(int)W||y>=(int)H) return;
    u8 *p=fb+(u64)y*PITCH+(u64)x*(BPP/8);
    if(BPP==32){p[0]=c&255;p[1]=(c>>8)&255;p[2]=(c>>16)&255;p[3]=255;}
    else if(BPP==24){p[0]=c&255;p[1]=(c>>8)&255;p[2]=(c>>16)&255;}
    else if(BPP==16){u16 v=((c>>19)&31)<<11|((c>>10)&63)<<5|((c>>3)&31); *(u16*)p=v;}
}
static void box(int x,int y,int w,int h,u32 fill,u32 edge){
    for(int j=y;j<y+h;j++) for(int i=x;i<x+w;i++) put(i,j,fill);
    for(int i=x;i<x+w;i++){put(i,y,edge);put(i,y+h-1,edge);} 
    for(int j=y;j<y+h;j++){put(x,j,edge);put(x+w-1,j,edge);} 
}
static void atext(int x,int y,const char*s,u32 c,int sc){
    for(;*s;s++){
        const unsigned char *g=font8[font_index((u8)*s)];
        for(int r=0;r<8;r++) for(int col=0;col<8;col++) if(g[r]&(1<<(7-col)))
            for(int sy=0;sy<sc;sy++) for(int sx=0;sx<sc;sx++) put(x+col*sc+sx,y+r*sc+sy,c);
        x+=9*sc;
    }
}
static int utf8(const u8**p){ const u8*s=*p; int c=s[0];
    if(c>=0xE0){*p=s+3; return ((c&0xF)<<12)|((s[1]&63)<<6)|(s[2]&63);}
    if(c>=0xC0){*p=s+2; return ((c&31)<<6)|(s[1]&63);}
    *p=s+1; return c; }
static void ctext(int x,int y,const char*s,u32 c,int sc){
    const u8*p=(const u8*)s;
    while(*p){ int cp=utf8(&p), gi=cjk_idx(cp);
        if(gi<0){x+=9*sc;continue;}
        for(int r=0;r<16;r++) for(int col=0;col<16;col++) if(cjk16[gi].rows[r]&(1<<col))
            for(int sy=0;sy<sc;sy++) for(int sx=0;sx<sc;sx++) put(x+col*sc+sx,y+r*sc+sy,c);
        x+=17*sc; }
}
static int tw(const char*s,int sc){ int n=0; for(;*s;s++) n+=9*sc; return n; }
static void ltext(int x,int y,const char*zh,const char*en,u32 c,int sc,int zh_ui){
    if(zh_ui&&zh) ctext(x,y,zh,c,sc); else atext(x,y,en,c,sc);
}
static void clear(u32 c){ for(u32 y=0;y<H;y++) for(u32 x=0;x<W;x++) put(x,y,c); }

/* ---- keyboard (PS/2 set 1) ---- */
static const char kmap[128]={
 0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b','\t',
 'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,'a','s',
 'd','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v',
 'b','n','m',',','.','/',0,'*',0,' ',0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static char key_get(void){
    while(!(inb(0x64)&1)) {}
    u8 sc=inb(0x60);
    if(sc&0x80) return 0;
    return kmap[sc];
}
static int key_wait(int timeout_ms10){
    for(int i=0;i<timeout_ms10;i++){
        if(inb(0x64)&1){ u8 sc=inb(0x60); if(!(sc&0x80)&&kmap[sc]) return kmap[sc]; }
        for(volatile int t=0;t<60000;t++);
    }
    return 0;
}

/* ---- cpuid ---- */
static void cpuid(u32 leaf,u32 *a,u32 *b,u32 *c,u32 *d){ __asm__ volatile("cpuid":"=a"(*a),"=b"(*b),"=c"(*c),"=d"(*d):"a"(leaf),"c"(0)); }
static char vendor[13]; static int cores=1;
static void cpu_probe(void){
    u32 a,b,c,d; cpuid(0,&a,&b,&c,&d);
    *(u32*)vendor=b; *(u32*)(vendor+4)=d; *(u32*)(vendor+8)=c; vendor[12]=0;
    cpuid(1,&a,&b,&c,&d); cores=(b>>16)&255;
    if(!cores) cores=1;
}
static char brand[49];
static void cpu_brand(void){
    u32 a,b,c,d; cpuid(0x80000000,&a,&b,&c,&d);
    if(a>=0x80000004){
        for(int i=0;i<3;i++){ cpuid(0x80000002+i,&a,&b,&c,&d); *(u32*)(brand+i*16)=a; *(u32*)(brand+i*16+4)=b; *(u32*)(brand+i*16+8)=c; *(u32*)(brand+i*16+12)=d; }
        brand[48]=0;
        for(int i=0;i<48;i++){ if(brand[i]<' ') brand[i]=' '; else if(brand[i]>='a'&&brand[i]<='z') brand[i]-=32; }
    } else { brand[0]=0; }
}

/* ---- multiboot2 ---- */
struct mb_tag { u32 type; u32 size; };
static u32 mem_upper_kb;
static u32 fb_addr, fb_pitch, fb_w, fb_h, fb_bpp, fb_type;
static void mb_parse(u32 addr){
    u32 total=*(u32*)addr;
    for(u32 off=8; off<total; ){
        struct mb_tag *t=(struct mb_tag*)(addr+off);
        if(t->type==4){ u32 *m=(u32*)(t+1); mem_upper_kb=m[1]; }
        else if(t->type==6){
            u8 *q=(u8*)(t+1); u32 esz=*(u32*)q; u32 ver=*(u32*)(q+4); u8 *e=q+8;
            u64 avail=0;
            if(esz>=24) for(; e+esz<=(u8*)t+t->size; e+=esz){
                u64 base=*(u64*)e, len=*(u64*)(e+8); u32 ty=*(u32*)(e+16);
                if(ty==1){ u64 end=base+len; if(end> (1u<<20)){ u64 st=base>(1u<<20)?base:(1u<<20); if(end>st) avail+=end-st; } }
            }
            if(avail){ mem_upper_kb=(u32)(avail>>10); }
        }
        else if(t->type==17){
            u8 *q=(u8*)(t+1); u32 dsz=*(u32*)q; u8 *e=q+8; u64 avail=0;
            if(dsz>=40) for(; e+dsz<=(u8*)t+t->size; e+=dsz){
                u32 ty=*(u32*)e; u64 pages=*(u64*)(e+24);
                if(ty==7) avail += pages*4096ull;
            }
            if(avail){ mem_upper_kb=(u32)(avail>>10); }
        }
        else if(t->type==8){
            u8 *f=(u8*)(t+1);
            fb_addr=*(u64*)f; fb_pitch=*(u32*)(f+8); fb_w=*(u32*)(f+12); fb_h=*(u32*)(f+16);
            fb_bpp=f[20]; fb_type=f[21];
        }
        else if(t->type==0) break;
        off += (t->size+7)&~7u;
    }
}

/* ---- ATA PIO ---- */
static void ata_wait(void){ for(int i=0;i<100000;i++){ u8 s=inb(0x1F7); if(!(s&0x80)) return; } }
static int ata_identify(u16 base,u8 drv,u64 *sectors,char *model){
    for(int i=0;i<41;i++) model[i]=0;
    outb(base+6,0xE0|(drv<<4)); for(int i=0;i<4;i++) inb(base+7);
    if(inb(base+7)==0xFF || inb(base+7)==0) return -1;
    outb(base+2,0); outb(base+3,0); outb(base+4,0); outb(base+5,0); outb(base+7,0xEC);
    io_wait(); u8 st=inb(base+7); if(st==0) return -1; ata_wait();
    if(!(inb(base+7)&8)) return -1;
    u16 buf[256]; for(int i=0;i<256;i++) buf[i]=inw(base);
    if(buf[0]==0x848A || buf[0]==0xEB14) return -1;   /* ATAPI (CD/DVD) */
    for(int i=0;i<20;i++){ model[i*2]=buf[27+i]>>8; model[i*2+1]=buf[27+i]&255; }
    model[40]=0;
    for(int i=0;i+3<=40;i++){ if(model[i]=='D'&&model[i+1]=='V'&&model[i+2]=='D') return -1; if(model[i]=='C'&&model[i+1]=='D'&&model[i+2]=='-') return -1; }
    *sectors=((u64)buf[61]<<16)|buf[60];
    if(!*sectors){ *sectors=((u64)buf[103]<<16|buf[100]) | ((u64)((buf[102]<<16)|buf[101])<<32); }
    return 0;
}
static int ata_read(u16 base,u8 drv,u32 lba,u8 *out){
    outb(base+6,0xE0|(drv<<4)|((lba>>24)&15));
    outb(base+2,1); outb(base+3,lba&255); outb(base+4,(lba>>8)&255); outb(base+5,(lba>>16)&255); outb(base+7,0x20);
    ata_wait(); for(int i=0;i<100000;i++){ u8 s=inb(base+7); if(s&8) break; if(s&1) return -1; }
    for(int i=0;i<256;i++){ u16 v=inw(base); out[i*2]=v&255; out[i*2+1]=v>>8; }
    return 0;
}

/* ---- disks ---- */
typedef struct { char model[41]; u64 sectors; u8 mbr[512]; int parts; } disk_t;
static disk_t disks[4]; static int ndisks;
static void disks_probe(void){
    u16 bases[2]={0x1F0,0x170};
    for(int b=0;b<2;b++) for(int d=0;d<2;d++){
        u64 sec=0; char m[41];
        if(ata_identify(bases[b],d,&sec,m)==0 && sec>=2048){
            ser_puts("ata: "); ser_puts(m); ser_puts(" sec="); ser_dec(sec); ser_putc('\n');
            disk_t *dk=&disks[ndisks]; *dk=(disk_t){0};
            for(int i=0;i<40;i++) dk->model[i]=m[i]; dk->sectors=sec;
            if(ndisks<4 && ata_read(bases[b],d,0,dk->mbr)==0){
                if(dk->mbr[510]==0x55 && dk->mbr[511]==0xAA){
                    for(int i=0;i<4;i++) if(dk->mbr[446+i*16+4]) dk->parts++;
                }
            }
            ndisks++;
        }
    }
}

/* ---- power ---- */
static void reboot_pc(void){ outb(0x64,0xFE); for(;;) __asm__ volatile("hlt"); }
static void poweroff_pc(void){
    outw(0x604,0x2000); outw(0xB004,0x2000);
    for(int i=0;i<1000;i++) outb(0x80,0);
    outw(0x604,0x2001);
    for(;;) __asm__ volatile("cli;hlt");
}

/* ---- screens ---- */
static int zh_ui=0;
static void draw_language(void){
    clear(BG);
    box(0,0,W,72,PANEL,EDGE);
    atext(24,18,"ALUMINUM",ACC,4);
    atext(200,26,"self-built kernel",DIM,2);
    atext(24,120,"Select language / choose one",TXT,2);
    box(W/2-360,H/2-70,320,140,PANEL,ACC);
    box(W/2+40,H/2-70,320,140,PANEL,WARN);
    atext(W/2-330,H/2-40,"[1] English",TXT,3);
    atext(W/2-330,H/2+10,"default, press 1",DIM,2);
    ctext(W/2+70,H/2-45,"中文",TXT,3);
    atext(W/2+70,H/2+10,"[2] Chinese",DIM,2);
    atext(24,H-40,"ALUMINUM 1.0  /  BIOS+UEFI  /  no Linux inside",DIM,1);
}

static void draw_gui(void){
    clear(BG);
    box(0,0,W,64,PANEL,EDGE);
    atext(24,16,"ALUMINUM 1.0",ACC,3);
    ltext(410,22,"系统控制台","SYSTEM CONSOLE",DIM,1,zh_ui);
    atext(W-150,26,"NO-LINUX",WARN,2);

    box(24,84,W-48,74,PANEL,EDGE);
    atext(40,96,"CPU",ACC,2);
    if(brand[0]) atext(120,96,brand,TXT,2);
    else { atext(120,96,vendor,TXT,2); }
    char b[80]; int n=0;
    /* cores */
    { char t[16]; int i=0; if(cores==0)t[i++]='0'; int c=cores; char r[8]; int k=0; while(c){r[k++]='0'+c%10;c/=10;} while(k)t[i++]=r[--k]; t[i]=0;
      for(int j=0;i>0;j++){ b[n++]=t[j%i]; if(j+1>=i)break; } }
    /* simpler: rebuild */
    n=0; { char t[12]; int i=0,c=cores; if(!c)t[i++]='0'; char r[8]; int k=0; while(c){r[k++]='0'+c%10;c/=10;} while(k)t[i++]=r[--k]; t[i]=0; for(int j=0;t[j];j++) b[n++]=t[j]; }
    ltext(40,130,"核心","CORES",DIM,2,zh_ui);
    atext(40+ (zh_ui?78:tw("CORES",2)+10),130,b,DIM,2);
    atext(330,130,"MEM",DIM,2);
    { char t[12]; int i=0,v=mem_upper_kb/1024; if(!v)t[i++]='0'; char r[8]; int k=0; while(v){r[k++]='0'+v%10;v/=10;} while(k)t[i++]=r[--k]; t[i]=0;
      atext(400,130,t,TXT,2); atext(400+ tw(t,2),130,"MB",DIM,2); }

    box(24,172,(W-60)/2,214,PANEL,EDGE);
    ltext(40,184,"磁盘","DISKS",ACC,2,zh_ui);
    int y=214;
    for(int i=0;i<ndisks && i<4;i++){
        char line[80]; int p=0;
        for(int j=0;disks[i].model[j] && j<24;j++) line[p++]=disks[i].model[j];
        line[p]=0;
        atext(40,y,line,TXT,2); y+=24;
        char sz[24]; u64 gb=disks[i].sectors/2048; int k=0; char r[24]; if(!gb)r[k++]='0'; while(gb){r[k++]='0'+gb%10;gb/=10;} while(k)sz[24-1-(--k)]=0;
        /* build size string */
        int m=0; char ss[24]; u64 g2=disks[i].sectors/2048; if(!g2) ss[m++]='0'; char rr[20]; int kk=0; while(g2){rr[kk++]='0'+g2%10;g2/=10;} while(kk) ss[m++]=rr[--kk]; ss[m]=0;
        atext(40,y,ss,DIM,2); atext(40+ tw(ss,2),y,"MB",DIM,2);
        char pc[16]; int q=0; pc[q++]='0'+disks[i].parts; pc[q]=0;
        atext(150,y,"parts",DIM,2); atext(240,y,pc,TXT,2);
        y+=30;
    }
    if(!ndisks) atext(40,214,"none",DIM,2);

    box(24+(W-60)/2+12,172,(W-60)/2,214,PANEL,EDGE);
    ltext(36+(W-60)/2+12,184,"安装","INSTALL",ACC,2,zh_ui);
    atext(36+(W-60)/2+12,220,"target: FAT32 volume",DIM,2);
    atext(36+(W-60)/2+12,250,"phase 2: writer + GRUB entry",DIM,2);
    atext(36+(W-60)/2+12,286,"boot: BIOS + UEFI",DIM,2);

    box(24,402,W-48,64,PANEL,EDGE);
    atext(40,418,"KEYS  [1]LANG  [2]REFRESH  [6]REBOOT  [7]POWEROFF",DIM,2);
    atext(40,442,"ALUMINUM 1.0  x86_64  /  independent kernel, no Linux",DIM,1);
    atext(24,H-30,"(C) ISXo / Aluminum project",DIM,1);
}

void kmain(u32 mb_addr){
    ser_init(); ser_puts("aluminum: entry\n");
    mb_parse(mb_addr);
    if(fb_addr){ fb=(u8*)fb_addr; W=fb_w; H=fb_h; PITCH=fb_pitch; BPP=fb_bpp; have_fb=1; }
    ser_puts("aluminum: fb="); ser_hex(fb_addr); ser_puts(" mode="); ser_dec(W); ser_putc('x'); ser_dec(H); ser_putc('x'); ser_dec(BPP); ser_putc('\n');
    cpu_probe(); cpu_brand(); disks_probe();
    ser_puts("aluminum: cpu="); ser_puts(vendor); ser_puts(" cores="); ser_dec(cores); ser_puts(" mem="); ser_dec(mem_upper_kb/1024); ser_puts("MB disks="); ser_dec(ndisks); ser_putc('\n');
    if(!have_fb){ ser_puts("aluminum: no framebuffer, halting\n"); for(;;) __asm__ volatile("hlt"); }

    draw_language();
    int k=0; for(int i=0;i<600 && !k;i++) k=key_wait(1);   /* ~6s default */
    if(k=='2'){ zh_ui=1; }
    ser_puts("aluminum: language="); ser_puts(zh_ui?"zh":"en"); ser_putc('\n');
    draw_gui();
    const char *msg="aluminum: gui up\n"; ser_puts(msg);

    for(;;){
        char c=key_get();
        if(!c) continue;
        if(c=='1'){ zh_ui=!zh_ui; draw_gui(); }
        else if(c=='2'||c=='r'||c=='R') draw_gui();
        else if(c=='6') reboot_pc();
        else if(c=='7') poweroff_pc();
    }
}
