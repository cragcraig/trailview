#ifndef FAT32_H
#define FAT32_H

#include <inttypes.h>

int init_partition(int part);

struct fat32fs_t
{
	uint32_t partition_begin_lba;
	uint32_t fat_begin_lba;
	uint32_t cluster_begin_lba;
	uint32_t root_dir_first_cluster;
	uint32_t sectors_per_fat;
	uint8_t sectors_per_cluster;
};

enum {DIRENT_FILE, DIRENT_EXTFILENAME, DIRENT_VOLLABEL, DIRENT_BLANK, DIRENT_END};

struct fat32dirent_t
{
	char filename[12];
	uint8_t attrib;
	uint32_t cluster;
	uint32_t size;
	uint32_t dcluster;

	char type;
};

struct fatwrite_t
{
	int sect_i;
	int sector_offset;
	uint32_t size;
	uint32_t f_cluster;
	uint32_t cur_cluster;
	uint32_t dir;
	uint8_t buf[512];
	char name[11];
};

/* the user level functions */

void ls(void);
void cd(const char * s);
void rn(const char * s, const char * snew);
void del(const char * s);
void cat(const char * s);
char exists(const char * s);
void touch(const char * s);
char mkdir(const char * dirname);
int dir_highestnumbered(void);

/* routines for writing to files */

char write_start(const char * s, struct fatwrite_t * fwrite);
void write_add(struct fatwrite_t * fwrite, const char * buf, int count);
void write_end(struct fatwrite_t * fwrite);
char write_append(const char * s, struct fatwrite_t * fwrite);

/* the workhorse functions */

#define IS_SUBDIR(dirent) (((dirent).attrib & 0x10) && ((dirent).type == DIRENT_FILE))
#define IS_FILE(dirent) ((dirent).type == DIRENT_FILE)

const char* str_to_fat(const char * str);
void loop_dir(uint32_t fcluster, char (*funct)(struct fat32dirent_t*));
void loop_file(uint32_t fcluster, int size, void (*funct)(uint8_t *, int));
uint32_t fat_findempty(void);
uint32_t fat_readnext(uint32_t cur_cluster);
uint32_t fat_writenext(uint32_t cur_cluster, uint32_t new_cluster);
void fat_clearchain(uint32_t first_cluster);
char print_dirent(struct fat32dirent_t* de);
void print_sect(uint8_t* s, int n);
char find_dirent(struct fat32dirent_t* de);
char find_emptyslot(struct fat32dirent_t* de);

#endif
