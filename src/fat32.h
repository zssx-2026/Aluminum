#ifndef FAT32_H
#define FAT32_H
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef unsigned long long u64;
typedef struct {
    int drv; u16 base; u32 part_lba;
    u16 bps; u8 spc; u16 reserved; u8 fats; u32 fat_size; u32 root_cluster; u32 total_sectors;
    u32 fat_start; u32 data_start; u32 clusters; u32 alloc_hint;
    u32 fsinfo_sector;
} fat32_t;
int fat32_mount(fat32_t *fs, int drv, u16 base, u32 part_lba);
u32 fat32_get(fat32_t *fs, u32 cl);
int fat32_set(fat32_t *fs, u32 cl, u32 val);
u32 fat32_alloc(fat32_t *fs);
int fat32_write_file(fat32_t *fs, const char *name11, const u8 *data, u32 len, u32 *first_out, u32 *size_out);
int fat32_list_root(fat32_t *fs, char names[][12], int max);
int fat32_write_file_ex(fat32_t *fs,u32 dir_cl,const char *name11,const u8 *data,u32 len,u32 *first_out,u32 *size_out);
int fat32_mkdir(fat32_t *fs,u32 parent_cl,const char *name11,u32 *out_cl);
u32 fat32_dir_lookup(fat32_t *fs,u32 dir_cl,const char *name11);
#endif
