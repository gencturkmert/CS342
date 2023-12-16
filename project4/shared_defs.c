#include "shared_defs.h"
// Initialize a BLOCK struct
struct BLOCK initBlock() {
    struct BLOCK block;
    memset(block.data, 0, BLOCKSIZE);
    return block;
}

// Initialize a DirEntry struct
struct DirEntry initDirEntry() {
    struct DirEntry dirEntry;
    dirEntry.empty = 0;
    memset(dirEntry.name, 0, MAX_FILENAME);
    dirEntry.file_size = 0;
    dirEntry.first_block = 0;
    memset(dirEntry.dummy, 0, DIR_ENTRY_SIZE - sizeof(int) * 2 - sizeof(char) * MAX_FILENAME);
    return dirEntry;
}

// Initialize a DirBlock struct
struct DirBlock initDirBlock() {
    struct DirBlock dirBlock;
    for (int i = 0; i < BLOCKSIZE / DIR_ENTRY_SIZE; ++i) {
        dirBlock.entries[i] = initDirEntry();
    }
    return dirBlock;
}

// Initialize a DirTableEntry struct
struct DirTableEntry initDirTableEntry() {
    struct DirTableEntry dirTableEntry;
    dirTableEntry.empty = 0;
    memset(dirTableEntry.name, 0, MAX_FILENAME);
    dirTableEntry.file_size = 0;
    dirTableEntry.first_block = 0;
    return dirTableEntry;
}

// Initialize a DirTable struct
struct DirTable initDirTable() {
    struct DirTable dirTable;
    for (int i = 0; i < MAX_DIR_COUNT; ++i) {
        dirTable.entries[i] = initDirTableEntry();
    }
    return dirTable;
}

// Initialize a FatEntry struct
struct FatEntry initFatEntry() {
    struct FatEntry fatEntry;
    fatEntry.empty = 0;
    fatEntry.next_block = 0;
    return fatEntry;
}

// Initialize a FatBlock struct
struct FatBlock initFatBlock() {
    struct FatBlock fatBlock;
    for (int i = 0; i < BLOCKSIZE / sizeof(struct FatEntry); ++i) {
        fatBlock.array[i] = initFatEntry();
    }
    return fatBlock;
}

// Initialize a FatTable struct
struct FatTable initFatTable() {
    struct FatTable fatTable;
    for (int i = 0; i < MAX_FAT_SIZE; ++i) {
        fatTable.entries[i] = initFatEntry();
    }
    return fatTable;
}

// Initialize an OftEntry struct
struct OftEntry initOftEntry() {
    struct OftEntry oftEntry;
    memset(oftEntry.filename, 0, MAX_FILENAME);
    oftEntry.dirIndex = -1;
    return oftEntry;
}

// Initialize an OFT struct
struct OFT initOft() {
    struct OFT oft;
    for (int i = 0; i < OPEN_FILE_TABLE_SIZE; ++i) {
        oft.entries[i] = initOftEntry();
    }
    return oft;
}