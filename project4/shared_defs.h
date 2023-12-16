#ifndef SHARED_DEFS_H
#define SHARED_DEFS_H

#define MAX_FILENAME 30 
#define DIR_ENTRY_SIZE 128
#define SUPERBLOCK_INDEX 0
#define DIR_START_INDEX 1
#define DIR_SIZE 7
#define FAT_START_INDEX 8
#define FAT_SIZE 32
#define MAX_FAT_SIZE 16384
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
    struct DirEntry entries[BLOCKSIZE/DIR_ENTRY_SIZE];
};

struct DirTableEntry{
    int empty;
    char name[MAX_FILENAME];
    int file_size;
    int first_block;
};

struct DirTable{

    struct DirTableEntry entries[MAX_DIR_COUNT];
};

//4 bytes
struct FatEntry {
    unsigned short empty; //0 empty 1 full
    unsigned short next_block;
};

struct FatBlock{
    struct FatEntry array[ BLOCKSIZE / sizeof( struct FatEntry) ];
};



struct FatTable{
    struct FatEntry entries[MAX_FAT_SIZE];
};

struct OftEntry{

    char filename[MAX_FILENAME];
    int dirIndex;
};

struct OFT{
    struct OftEntry entries[16];
};

#endif


