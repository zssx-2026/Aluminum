#include "fat32.h"
extern int ata_read(u16 base,u8 drv,u32 lba,u8 *out);
extern int ata_write(u16 base,u8 drv,u32 lba,const u8 *in);

static u8 sec[512];
static int rd(fat32_t *fs,u32 lba,u8 *buf){ return ata_read(fs->base,fs->drv,lba,buf); }
static int wr(fat32_t *fs,u32 lba,const u8 *buf){ return ata_write(fs->base,fs->drv,lba,buf); }
static u32 cl_sector(fat32_t *fs,u32 cl){ return fs->data_start + (cl-2)*fs->spc; }

int fat32_mount(fat32_t *fs,int drv,u16 base,u32 part_lba){
    fs->drv=drv; fs->base=base; fs->part_lba=part_lba;
    if(rd(fs,part_lba,sec)) return -1;
    if(sec[510]!=0x55||sec[511]!=0xAA) return -2;
    u16 bps=*(u16*)(sec+11); u8 spc=sec[13]; u16 res=*(u16*)(sec+14); u8 nf=sec[16];
    u32 tot=*(u16*)(sec+19); if(!tot) tot=*(u32*)(sec+32);
    u32 fsz=*(u16*)(sec+22); if(!fsz) fsz=*(u32*)(sec+36);
    u32 root=*(u32*)(sec+44);
    if(bps!=512||spc==0||fsz==0) return -3;
    fs->bps=bps; fs->spc=spc; fs->reserved=res; fs->fats=nf; fs->fat_size=fsz; fs->root_cluster=root; fs->total_sectors=tot;
    fs->fat_start=part_lba+res; fs->data_start=fs->fat_start+nf*fsz;
    u32 fatcap=fsz*512/4-2;
    fs->clusters=(tot-res-nf*fsz)/spc;
    if(fs->clusters>fatcap) fs->clusters=fatcap;
    fs->alloc_hint=2;
    fs->fsinfo_sector=part_lba+(*(u16*)(sec+48));
    return 0;
}

u32 fat32_get(fat32_t *fs,u32 cl){
    u32 off=cl*4, secno=fs->fat_start+(off>>9);
    if(rd(fs,secno,sec)) return 0x0FFFFFFF;
    return (*(u32*)(sec+(off&511))) & 0x0FFFFFFF;
}

int fat32_set(fat32_t *fs,u32 cl,u32 val){
    u32 off=cl*4;
    for(u8 i=0;i<fs->fats;i++){
        u32 secno=fs->fat_start+i*fs->fat_size+(off>>9);
        if(rd(fs,secno,sec)) return -1;
        *(u32*)(sec+(off&511))=(*(u32*)(sec+(off&511))&0xF0000000)|(val&0x0FFFFFFF);
        if(wr(fs,secno,sec)) return -1;
    }
    return 0;
}

u32 fat32_alloc(fat32_t *fs){
    for(u32 c=fs->alloc_hint;c<fs->clusters+2;c++) if(fat32_get(fs,c)==0){ fat32_set(fs,c,0x0FFFFFFF); fs->alloc_hint=c+1; return c; }
    for(u32 c=2;c<fs->alloc_hint;c++) if(fat32_get(fs,c)==0){ fat32_set(fs,c,0x0FFFFFFF); fs->alloc_hint=c+1; return c; }
    return 0;
}

static u32 dir_find(fat32_t *fs,const char *name11,u32 *prev,u32 *prev_off){
    /* return cluster of dirent slot and offset, or 0 */
    u32 cl=fs->root_cluster;
    while(cl>=2&&cl<0x0FFFFFF8){
        for(u32 s=0;s<fs->spc;s++){
            if(rd(fs,cl_sector(fs,cl)+s,sec)) return 0;
            for(int i=0;i<16;i++){
                u8 *e=sec+i*32;
                if(e[0]==0x00){ if(prev){*prev=cl;*prev_off=i;} return 0; }
                if(e[0]==0xE5){ if(prev){*prev=cl;*prev_off=i;} continue; }
                int ok=1;
                for(int k=0;k<11;k++) if(e[k]!=name11[k]){ok=0;break;}
                if(ok) return cl*1000+s*16+i;
            }
        }
        cl=fat32_get(fs,cl);
    }
    return 0;
}

int fat32_write_file_ex(fat32_t *fs,u32 dir_cl,const char *name11,const u8 *data,u32 len,u32 *first_out,u32 *size_out){
    u32 cl=dir_cl; u32 free_cl=0,fsec=0,fo=0; int found=0;
    while(cl>=2&&cl<0x0FFFFFF8&&!found){
        for(u32 s=0;s<fs->spc&&!found;s++){
            if(rd(fs,cl_sector(fs,cl)+s,sec)) return -1;
            for(int i=0;i<16;i++){
                u8 *e=sec+i*32;
                if(e[0]==0x00||e[0]==0xE5){ free_cl=cl; fsec=s; fo=i; found=1; break; }
            }
        }
        if(!found) cl=fat32_get(fs,cl);
    }
    if(!found) return -2;

    u32 need=(len+fs->spc*512-1)/(fs->spc*512); if(!need) need=1;
    u32 first=0,prev=0;
    for(u32 n=0;n<need;n++){
        u32 c=fat32_alloc(fs);
        if(!c) return -3;
        if(!first) first=c; else fat32_set(fs,prev,c);
        prev=c;
        u32 base=cl_sector(fs,c);
        for(u32 s=0;s<fs->spc;s++){
            u32 off=(n*fs->spc+s)*512;
            for(int b=0;b<512;b++) sec[b]=(off+(u32)b<len)?data[off+b]:0;
            if(wr(fs,base+s,sec)) return -4;
        }
    }
    /* write dirent */
    if(rd(fs,cl_sector(fs,free_cl)+fsec,sec)) return -5;
    u8 *e=sec+fo*32;
    for(int i=0;i<11;i++) e[i]=name11[i];
    e[11]=0x20; e[12]=0; e[13]=0; e[14]=0; e[15]=0;
    u16 dt=(u16)(((2026-1980)<<9)|(1<<5)|1);
    *(u16*)(e+16)=dt; *(u16*)(e+18)=dt; *(u16*)(e+24)=dt;
    *(u16*)(e+20)=(u16)(first>>16); *(u16*)(e+26)=(u16)(first&0xFFFF); *(u32*)(e+28)=len;
    if(wr(fs,cl_sector(fs,free_cl)+fsec,sec)) return -6;
    if(first_out)*first_out=first; if(size_out)*size_out=len;
    return 0;
}

int fat32_list_root(fat32_t *fs,char names[][12],int max){
    int n=0; u32 cl=fs->root_cluster;
    while(cl>=2&&cl<0x0FFFFFF8&&n<max){
        for(u32 s=0;s<fs->spc;s++){
            if(rd(fs,cl_sector(fs,cl)+s,sec)) return n;
            for(int i=0;i<16;i++){
                u8 *e=sec+i*32;
                if(e[0]==0x00) return n;
                if(e[0]==0xE5||(e[11]&0x0F)==0x0F) continue;
                for(int k=0;k<11;k++) names[n][k]=e[k];
                names[n][11]=0; n++;
                if(n>=max) return n;
            }
        }
        cl=fat32_get(fs,cl);
    }
    return n;
}

int fat32_write_file(fat32_t *fs,const char *name11,const u8 *data,u32 len,u32 *fo,u32 *so){
    return fat32_write_file_ex(fs,fs->root_cluster,name11,data,len,fo,so);
}

static u32 find_free_slot(fat32_t *fs,u32 dir_cl,u32 *out_sec,u32 *out_off){
    u32 cl=dir_cl;
    while(cl>=2&&cl<0x0FFFFFF8){
        for(u32 s=0;s<fs->spc;s++){
            if(rd(fs,cl_sector(fs,cl)+s,sec)) return 0;
            for(int i=0;i<16;i++){
                u8 *e=sec+i*32;
                if(e[0]==0x00||e[0]==0xE5){ *out_sec=cl_sector(fs,cl)+s; *out_off=i; return 1; }
            }
        }
        cl=fat32_get(fs,cl);
    }
    return 0;
}

int fat32_mkdir(fat32_t *fs,u32 parent_cl,const char *name11,u32 *out_cl){
    u32 s,l; if(!find_free_slot(fs,parent_cl,&s,&l)) return -1;
    u32 c=fat32_alloc(fs); if(!c) return -2;
    for(u32 i=0;i<fs->spc;i++){
        for(int b=0;b<512;b++) sec[b]=0;
        if(i==0){
            u8 *dot=sec; for(int k=0;k<11;k++) dot[k]=(k==0)?'.':0; dot[11]=0x10;
            *(u16*)(dot+26)=(u16)c; *(u16*)(dot+20)=0;
            u8 *dd=sec+32; dd[0]='.'; dd[1]='.'; for(int k=2;k<11;k++) dd[k]=0; dd[11]=0x10;
            *(u16*)(dd+26)=(u16)(parent_cl==fs->root_cluster?0:parent_cl);
        }
        if(wr(fs,cl_sector(fs,c)+i,sec)) return -3;
    }
    if(rd(fs,s,sec)) return -4;
    u8 *e=sec+l*32; for(int k=0;k<11;k++) e[k]=name11[k];
    e[11]=0x10; e[12]=0; e[13]=0; e[14]=0; e[15]=0;
    *(u16*)(e+20)=(u16)(c>>16); *(u16*)(e+26)=(u16)c; *(u32*)(e+28)=0;
    if(wr(fs,s,sec)) return -5;
    if(out_cl)*out_cl=c; return 0;
}

u32 fat32_dir_lookup(fat32_t *fs,u32 dir_cl,const char *name11){
    u32 cl=dir_cl;
    while(cl>=2&&cl<0x0FFFFFF8){
        for(u32 s=0;s<fs->spc;s++){
            if(rd(fs,cl_sector(fs,cl)+s,sec)) return 0;
            for(int i=0;i<16;i++){
                u8 *e=sec+i*32;
                if(e[0]==0x00) return 0;
                if(e[0]==0xE5||(e[11]&0x0F)==0x0F) continue;
                int ok=1; for(int k=0;k<11;k++) if(e[k]!=name11[k]){ok=0;break;}
                if(ok) return (u32)(*(u16*)(e+20)<<16)|*(u16*)(e+26);
            }
        }
        cl=fat32_get(fs,cl);
    }
    return 0;
}
