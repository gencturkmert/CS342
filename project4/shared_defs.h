#ifndef SHARED_DEFS_H
#define SHARED_DEFS_H

#define MAX_FILENAME 30 
#define DIR_ENTRY_SIZE 128
#define SUPERBLOCK_INDEX 0
#define FAT_START_INDEX 1
#define FAT_SIZE 32
#define MAX_FAT_SIZE 16384
#define FAT_ENTRIES_PER_BLOCK 512
#define DIR_START_INDEX 33
#define DIR_BLOCK_SIZE 8
#define DIR_BLOCK_ENTRY_COUNT 16
#define DATA_BLOCKS_START 41
#define MAX_DIR_COUNT 128
#define MAX_DISK_SIZE 23
#define MIN_DISK_SIZE 18
#define OPEN_FILE_TABLE_SIZE 16
#define BLOCKSIZE 2048

struct BLOCK {
    char data[BLOCKSIZE];
};

struct DirEntry{
  int empty;
  char name[ MAX_FILENAME ];
  int  file_size;
  int  first_block;
  
  char dummy[ DIR_ENTRY_SIZE - sizeof(int)*2 - sizeof(char)*MAX_FILENAME ];
};

struct DirBlock{
    struct DirEntry entries[DIR_BLOCK_ENTRY_COUNT];
};



struct DirTable{
    struct DirBlock entries[DIR_BLOCK_SIZE];
};

//4 bytes
struct FatEntry {
    unsigned short empty; //0 empty 1 full
    unsigned short next_block;
};

struct FatBlock{
    struct FatEntry array[ FAT_ENTRIES_PER_BLOCK];
};

struct FatBlockTable{
    struct FatBlock blocks[FAT_SIZE];
};

struct FatTable{
    struct FatEntry entries[MAX_FAT_SIZE];
};

struct OftEntry{

    char filename[MAX_FILENAME];
    int dirIndex;
};

struct OFT{
    struct OftEntry entries[OPEN_FILE_TABLE_SIZE];
};

struct BLOCK initBlock();
struct DirEntry initDirEntry();
struct DirBlock initDirBlock();
struct DirTable initDirTable();
struct FatEntry initFatEntry();
struct FatBlock initFatBlock();
struct FatTable initFatTable();
struct FatBlockTable initFatBlockTable();
struct OftEntry initOftEntry();
struct OFT initOft();

#endif


