#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "vsfs.h"
#include "shared_defs.h"


// globals  =======================================
int vs_fd; // file descriptor of the Linux file that acts as virtual disk.
              // this is not visible to an application.
// ========================================================

struct BLOCK superBlock;
struct FatTable fatTable;
struct FatBlockTable fatBlockTable;
struct DirTable dirTable;
struct OFT oftTable;

// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk. 
int read_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vs_fd, (off_t) offset, SEEK_SET);
    n = read (vs_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
	printf ("read error\n");
	return -1;
    }
    return (0); 
}

// write block k into the virtual disk. 
int write_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vs_fd, (off_t) offset, SEEK_SET);
    n = write (vs_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
	printf ("write error\n");
	return (-1);
    }
    return 0; 
}


/**********************************************************************
   The following functions are to be called by applications directly. 
***********************************************************************/

// this function is partially implemented.
int vsformat (char *vdiskname, unsigned int m)
{

    if(m > MAX_DISK_SIZE || m < MIN_DISK_SIZE) {
        printf("Disksize must be between %d and %d\n",MIN_DISK_SIZE,MAX_DISK_SIZE);
    }
    char command[1000];
    int size;
    int num = 1;
    int count;
    size  = num << m;
    count = size / BLOCKSIZE;
    sprintf (command, "dd if=/dev/zero of=%s bs=%d count=%d",
             vdiskname, BLOCKSIZE, count);
    system (command);

    if(vsmount(vdiskname) == 0 ) {
        printf("Disk formatted and mounted\n");
        return( 0);
    }

    return (-1); 
}


// this function is partially implemented.
int  vsmount (char *vdiskname)
{
    // open the Linux file vdiskname and in this
    // way make it ready to be used for other operations.
    // vs_fd is global; hence other function can use it. 
    vs_fd = open(vdiskname, O_RDWR);

    if (read_block(&superBlock, 0) == -1) {
        close(vs_fd);
        printf("Error reading superblock, virtual disk closed\n");
        return -1;
    }

    oftTable = initOft();
    fatTable = initFatTable();
    fatBlockTable = initFatBlockTable();
    dirTable = initDirTable();

    for (int i = 0; i < FAT_SIZE; ++i) {
        struct FatBlock fatblock;
        if (read_block(&fatblock, FAT_START_INDEX + i) == -1) {
            printf("Error reading FAT table block %d, closing virtual disk\n",i+1);
            close(vs_fd);
            return -1;
        }

        fatBlockTable.blocks[i] = fatblock;

        for (int j = 0; j < FAT_ENTRIES_PER_BLOCK; ++j) {
            fatTable.entries[i * FAT_ENTRIES_PER_BLOCK + j] = fatblock.array[j];
        }

    }

    for (int i = 0; i < DIR_BLOCK_SIZE; ++i) {
        struct DirBlock dirBlock;
        if (read_block(&dirBlock, DIR_START_INDEX + i) == -1) {
            close(vs_fd);
            printf("Error reading directory block %d, closing virtual disk\n",i);
            return -1;
        }

        dirTable.entries[i] = dirBlock;
    }

    printf("Virtual disk mounted\n");
    return(0);
}


// this function is partially implemented.
int vsumount() {

    if (write_block(&superBlock, 0) == -1) {
        printf("Error writing superblock\n");
        return -1;
    }

    for (int i = 0; i < FAT_SIZE; ++i) {
        if (write_block(&(fatBlockTable.blocks[i]), FAT_START_INDEX + i) == -1) {
            printf("Error writing FAT block\n");
            return -1;
        }
    }

    for (int i = 0; i < DIR_BLOCK_SIZE; ++i) {
        if (write_block(&dirTable.entries[i], DIR_START_INDEX + i) == -1) {
            printf("Error writing directory block\n");
            return -1;
        }
    }

    fsync(vs_fd);

    if (close(vs_fd) == -1) {
        perror("Error closing virtual disk\n");
        return -1;
    }

    return 0;
}


int vscreate(char *filename)
{
    for (int i = 0; i < DIR_BLOCK_SIZE; ++i) {
        for (int j = 0; j < DIR_BLOCK_ENTRY_COUNT; ++j) {
            if (dirBlockTable.entries[i].entries[j].empty == 0) {


                int found = 0;
                int firstIndex = -1;
                int secondIndex = -1;
                for (int k = 0; k < FAT_SIZE; ++k) {
                    for (int l = 0; l < FAT_ENTRIES_PER_BLOCK; ++l) {
                 
                        if (fatTable->blocks[k].array[l].empty == 0) {
                            firstIndex = k;
                            secondIndex = l;
                           found = 1;
                           break;
                        }
                    }

                    if(found == 1) {
                        break;
                    }
                }

                if(found == 1) {
                    struct DirEntry newFileEntry = dirBlockTable.entries[i].entries[j];
                    newFileEntry.empty = 1;  
                    strncpy(newFileEntry->name, filename, MAX_FILENAME);
                    newFileEntry.file_size = 0;  
                    newFileEntry.first_block = firstIndex * FAT_ENTRIES_PER_BLOCK + secondIndex;
                    fatTable.blocks[firstIndex].array[secondIndex].next_block = -1;
                    fatTable.blocks[firstIndex].array[secondIndex].empty = 1;
                    dirBlockTable.entries[i].entries[j]=newFileEntry;
                    dirTable.entries[i*DIR_BLOCK_SIZE+j] = newFileEntry;

                    return 0;
                }else{
                    printf("There is no empty block for file\n");
                    return 0;
                }


            }
        }
    }

    printf("Error: No empty file slot available in the directory table.\n");
    return -1;
}


int vsopen(char *file, int mode)
{

    for (int i = 0; i < DIR_BLOCK_SIZE; ++i) {
        for (int j = 0; j < DIR_BLOCK_ENTRY_COUNT; ++j) {
            if (dirTable.entries[i].entries[j].empty == 1 &&
                strcmp(dirTable.entries[i].entries[j].name, file) == 0) {

                    for (int k = 0; k < OPEN_FILE_TABLE_SIZE; ++k) {
                        if (oftTable.entries[k].dirIndex == -1) {

                            oftTable.entries[k].dirIndex = i * DIR_BLOCK_ENTRY_COUNT + j;
                            strncpy(oftTable.entries[k].filename, file, MAX_FILENAME);
                            oftTable.entries[k].currentBlock = dirTable.entries[i].entries[j].first_block;
                            oftTable.entries[k].currentOffset = 0;

                            return k;
                        }
                    }

                    printf("There is no space in directory, file could not opened, close a file first\n");
                    return -1;
                    
                }
        }
    }
    
    printf("File could not been found, create file first\n");
    return -1;
}

int vsclose(int fd){
    return (0); 
}

int vssize (int  fd)
{
    int dirIndex = oftTable.entries[fd].dirIndex;
    struct DirEntry entry =  dirTable.entries[dirIndex / DIR_BLOCK_ENTRY_COUNT].entries[dirIndex % DIR_BLOCK_ENTRY_COUNT];
    return entry.file_size;
}

int vsread(int fd, void *buf, int n){
    return (0); 
}


int vsappend(int fd, void *buf, int n)
{
    return (0); 
}

int vsdelete(char *filename)
{
    return (0); 
}

