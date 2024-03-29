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

int read_block_offset (void *block, int k, int o, int size)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE + o;
    lseek(vs_fd, (off_t) offset, SEEK_SET);
    n = read (vs_fd, block, size);
    if (n != size) {
	printf ("read error while reading offset, size %d\n",n);
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

int write_block_offset (void *block, int k, int o, int size)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE + o;
    lseek(vs_fd, (off_t) offset, SEEK_SET);
    n = write (vs_fd, block, size);

    if(n != size) {
        printf("Write error o\n");
        return -1;
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
            if (dirTable.entries[i].entries[j].empty == 0) {

                int newBlock = allocateNewBlock(-1);

                if(newBlock != -1) {
                    struct DirEntry newFileEntry = dirTable.entries[i].entries[j];
                    newFileEntry.empty = 1;  
                    strncpy(newFileEntry.name, filename, MAX_FILENAME);
                    newFileEntry.file_size = 0;  
                    newFileEntry.first_block = newBlock;
                    dirTable.entries[i].entries[j]=newFileEntry;

                    printf("File %s created succesfully\n",filename);
                    return 0;
                }else{
                    printf("There is no empty block for file\n");
                    return -1;
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
                            oftTable.entries[k].mode = mode;

                            if(mode == MODE_APPEND) {
                                if(dirTable.entries[i].entries[j].file_size >0) {
                                    int size = dirTable.entries[i].entries[j].file_size;
                                    int blockCount = size / BLOCKSIZE;
                                    short lastBlock = dirTable.entries[i].entries[j].first_block;
                                    while(blockCount != 0) {
                                        int nextBlock = getNextBlock(lastBlock);

                                        if(nextBlock == -1) {
                                            break;
                                        }

                                        blockCount-=1;
                                    }

                                    oftTable.entries[k].currentBlock = lastBlock;
                                    oftTable.entries[k].currentOffset = size % BLOCKSIZE;


                                }
                            }

                            printf("File %s opened succesfully\n",file);
                            return k;
                        }
                    }

                    printf("There should be maximum 16 files opened, file could not opened, close a file first\n");
                    return -1;
                    
                }
        }
    }
    
    printf("File could not been found, create file first\n");
    return -1;
}

int vsclose(int fd){
    if (fd >= 0 && fd < OPEN_FILE_TABLE_SIZE && oftTable.entries[fd].dirIndex != -1 ) {
        oftTable.entries[fd].dirIndex = -1;
        printf("File with descriptor %d closed successfully.\n", fd);
        return 0;
    } else {
        printf("Error: Invalid file descriptor %d.\n", fd);
        return -1;
    }
}

int vssize (int  fd)
{
    int dirIndex = oftTable.entries[fd].dirIndex;
    return dirTable.entries[dirIndex / DIR_BLOCK_ENTRY_COUNT].entries[dirIndex % DIR_BLOCK_ENTRY_COUNT].file_size;
    
}

int vsread(int fd, void *buf, int n)
{
    struct OftEntry* oftEntry = &oftTable.entries[fd];

    if(oftEntry->dirIndex == -1) {
        printf("Error in file read, file is not found in oft\n");
        return -1;
    }

    if(oftEntry->mode == MODE_APPEND) {
        printf("File is opened in append mode, can't read values\n");
        return -1;
    }

    if(n>BLOCKSIZE) {
        printf("Maximum %d bytes can be read in single call\n",BLOCKSIZE);
        return -1;
    }

    if(oftEntry->currentBlock == -1) {
        printf("File ended\n");
        return -1;
    }

    

    int remainingSpace = BLOCKSIZE - oftEntry->currentOffset;

    if(remainingSpace > n) {
        read_block_offset(buf,oftEntry->currentBlock,oftEntry->currentOffset,n);
        oftEntry->currentOffset += n;
    }else{
        void *nextBuf = buf;
        if(remainingSpace != 0) {
            read_block_offset(buf, oftEntry->currentBlock, oftEntry->currentOffset, remainingSpace);
            nextBuf =  (void*)((char*)buf + remainingSpace);
            n = n - remainingSpace;
            oftEntry->currentOffset = 0;
        }

        int newBlockIndex = getNextBlock(oftEntry->currentBlock);
        oftEntry->currentBlock = newBlockIndex;
        if (newBlockIndex == -1) {
            printf("Error: Unable to read next block. File ended\n");
            return -1;
        }

        
        read_block_offset(nextBuf, oftEntry->currentBlock, oftEntry->currentOffset, n);
        oftEntry->currentOffset += n;
          
    }
   // printf("Current offset %d\n",oftEntry->currentOffset);


    return 0;
}


int vsappend(int fd, void *buf, int n)

{
    struct OftEntry* oftEntry = &oftTable.entries[fd];
    if(oftEntry->dirIndex == -1) {
        printf("Error in file append, file is not found in oft\n");
        return -1;
    }

    if(oftEntry->mode == MODE_READ) {
        printf("File is opened in read mode, can't append values\n");
        return -1;
    }

    if(n>BLOCKSIZE) {
        printf("Maximum %d bytes can be written in single call\n",BLOCKSIZE);
        return -1;
    }

    int remainingSpace = BLOCKSIZE - oftEntry->currentOffset;
    //printf("Remaining space is %d\n",remainingSpace);

    if(remainingSpace > n) {
        write_block_offset(buf,oftEntry->currentBlock,oftEntry->currentOffset,n);
        oftEntry->currentOffset += n;
    }else{
        if(remainingSpace != 0) {
            write_block_offset(buf, oftEntry->currentBlock, oftEntry->currentOffset, remainingSpace);
            buf =  (void*)((char*)buf + remainingSpace);
            n = n - remainingSpace;
            oftEntry->currentOffset = 0;
        }

        int newBlockIndex = allocateNewBlock(oftEntry->currentBlock);
        if (newBlockIndex == -1) {
            printf("Error: Unable to allocate a new block.\n");
            return -1;
        }

        oftEntry->currentBlock = newBlockIndex;
        
        write_block_offset(buf, oftEntry->currentBlock, oftEntry->currentOffset, n);
        oftEntry->currentOffset += n;
          
    }


    dirTable.entries[oftEntry->dirIndex/DIR_BLOCK_ENTRY_COUNT].entries[oftEntry->dirIndex%DIR_BLOCK_ENTRY_COUNT].file_size += n;
    //printf("File size incremented %d\n",dirTable.entries[oftEntry->dirIndex/DIR_BLOCK_ENTRY_COUNT].entries[oftEntry->dirIndex%DIR_BLOCK_ENTRY_COUNT].file_size);

    return 0;

}

int vsdelete(char *filename)
{

     for (int i = 0; i < DIR_BLOCK_SIZE; ++i) {
        for (int j = 0; j < DIR_BLOCK_ENTRY_COUNT; ++j) {
            if (dirTable.entries[i].entries[j].empty == 1 &&
                strcmp(dirTable.entries[i].entries[j].name, filename) == 0) {

                    for (int k = 0; k < OPEN_FILE_TABLE_SIZE; ++k) {
                        if (strcmp(oftTable.entries[k].filename, filename) == 0) {

                            oftTable.entries[k].dirIndex = -1;;
                            oftTable.entries[k].currentBlock = -1;
                            oftTable.entries[k].currentOffset = 0;
                            oftTable.entries[k].mode = -1;

                            
                        }
                    }

                    dirTable.entries[i].entries[j].empty = 0;
                    dirTable.entries[i].entries[j].file_size = 0;
                    deleteBlock(dirTable.entries[i].entries[j].first_block);
                    dirTable.entries[i].entries[j].first_block = -1;
                    printf("File deleted \n");
                    return 0;
                    
                }
        }
    }
    
    printf("File could not been found, create file first\n");
    return -1;
}

int deleteBlock(int currIndex) {
    int nextIndex = fatBlockTable.blocks[currIndex/FAT_ENTRIES_PER_BLOCK].array[currIndex%FAT_ENTRIES_PER_BLOCK].next_block;
    fatBlockTable.blocks[currIndex/FAT_ENTRIES_PER_BLOCK].array[currIndex%FAT_ENTRIES_PER_BLOCK].empty = 0;
    fatBlockTable.blocks[currIndex/FAT_ENTRIES_PER_BLOCK].array[currIndex%FAT_ENTRIES_PER_BLOCK].next_block = -1;
    
    if(nextIndex != -1) {
        deleteBlock(nextIndex);
    }

    return 0;
}

int allocateNewBlock(int prevBlockIndex) {
    printf("prevBlockIndex %d",prevBlockIndex);
    for (int i = 0; i < FAT_SIZE; ++i) {
        for (int j = 0; j < FAT_ENTRIES_PER_BLOCK; ++j) {
            if (fatBlockTable.blocks[i].array[j].empty == 0) {
                fatBlockTable.blocks[i].array[j].empty = 1;
                fatBlockTable.blocks[i].array[j].next_block = -1; 

                if(prevBlockIndex != -1) {
                    fatBlockTable.blocks[prevBlockIndex/FAT_ENTRIES_PER_BLOCK].array[prevBlockIndex%FAT_ENTRIES_PER_BLOCK].next_block = i * FAT_ENTRIES_PER_BLOCK + j;
                }
                printf("Block %d allocated\n",i * FAT_ENTRIES_PER_BLOCK + j);
                return i * FAT_ENTRIES_PER_BLOCK + j;
            }
        }
    }

    printf("No empty block found in fat table\n");
    return -1;
}

int getNextBlock(int currentBlock) {
    printf("Current Block %d\n",currentBlock);
    if(currentBlock/FAT_ENTRIES_PER_BLOCK > 32) {
        return -1;
    }

    int nextIndex = fatBlockTable.blocks[currentBlock/FAT_ENTRIES_PER_BLOCK].array[currentBlock%FAT_ENTRIES_PER_BLOCK].next_block;
    printf("Next block %d\n",nextIndex);
    return nextIndex;
}

