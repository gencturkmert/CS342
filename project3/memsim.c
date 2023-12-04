#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <limits.h>
#include "ClockList.h"

#define PAGE_SIZE 64
#define VIRTUAL_MEMORY_SIZE 1024

#define SECOND_LEVEL_TABLE_SIZE 32

#define V_MASK 0x8000
#define R_MASK 0x4000
#define M_MASK 0x2000

enum PageReplacementAlgorithm
{
    FIFO,
    LRU,
    CLOCK,
    ECLOCK
};

// V+R+M+ UNUSED BITS + K BITS
// V = 0 is invalid
struct PageTableEntry
{
    unsigned short int bits;
    int clock;
};

struct FirstLevelPageTable
{
    struct PageTableEntry entries[VIRTUAL_MEMORY_SIZE];
};
typedef struct
{
    char chars[PAGE_SIZE];
} Data;

struct SecondLevelPageTable
{
    struct FirstLevelPageTable tables[SECOND_LEVEL_TABLE_SIZE];
};

typedef struct
{
    int size;
    Data *data;
} RAM;

int level;
char addrfile[100];
char swapfile[100];
int fcount;
char algo[10];
int tick;
char outfile[100];

RAM ram;

// For fifo implementation
unsigned int recent = 0;

// for lru implementation
int clock = 0;

int totalPageFault = 0;

int hasNonNullOrNot(const char *str)
{
    for (int i = 0; i < PAGE_SIZE; ++i)
    {
        if (str[i] != '0')
        {
            return 1;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc != 15)
    {
        printf("Error: Incorrect number of arguments\n");
        printf("Usage: memsim -p level -r addrfile -s swapfile -f fcount -a algo -t tick -o outfile\n");
        return 1;
    }

    for (int i = 1; i < argc; i += 2)
    {
        if (strcmp(argv[i], "-p") == 0)
        {
            level = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-r") == 0)
        {
            strncpy(addrfile, argv[i + 1], sizeof(addrfile));
        }
        else if (strcmp(argv[i], "-s") == 0)
        {
            strncpy(swapfile, argv[i + 1], sizeof(swapfile));
        }
        else if (strcmp(argv[i], "-f") == 0)
        {
            fcount = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-a") == 0)
        {
            strncpy(algo, argv[i + 1], sizeof(algo));
        }
        else if (strcmp(argv[i], "-t") == 0)
        {
            tick = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-o") == 0)
        {
            strncpy(outfile, argv[i + 1], sizeof(outfile));
        }
        else
        {
            printf("Error: Unknown option %s\n", argv[i]);
            printf("Usage: memsim -p level -r addrfile -s swapfile -f fcount -a algo -t tick -o outfile\n");
            return 1;
        }
    }

    printf("level: %d\n", level);
    printf("addrfile: %s\n", addrfile);
    printf("swapfile: %s\n", swapfile);
    printf("fcount: %d\n", fcount);
    printf("algo: -%s-\n", algo);
    printf("tick: %d\n", tick);
    printf("outfile: %s\n", outfile);

    FILE *input = fopen(addrfile, "r");
    if (input == NULL)
    {
        printf("Error opening addrfile");
        return 1;
    }

    FILE *disc;

    disc = fopen(swapfile, "w+b");

    if (disc == NULL)
    {
        printf("Error creating swapfile");
        exit(1);
    }

    // all zeros
    unsigned char zeroBuffer = 0;

    for (int i = 0; i < VIRTUAL_MEMORY_SIZE * PAGE_SIZE; ++i)
    {
        fwrite(&zeroBuffer, sizeof(char), 1, disc);
    }

    FILE *output = fopen(outfile, "w");
    if (output == NULL)
    {
        printf("Error opening outfile");
        fclose(input);
        fclose(disc);
        return 1;
    }

    ram.size = fcount;
    ram.data = (Data *)malloc(fcount * sizeof(Data));
    if (ram.data == NULL)
    {
        fprintf(stderr, "Memory allocation for RAM data failed.\n");
        fclose(input);
        fclose(disc);
        fclose(output);
        return 1;
    }

    for (int i = 0; i < fcount; ++i)
    {
        memset(ram.data[i].chars, '0', PAGE_SIZE);
    }

    struct FirstLevelPageTable firstLevelPageTable;
    if (level == 1)
    {
        for (int i = 0; i < VIRTUAL_MEMORY_SIZE; i++)
        {
            firstLevelPageTable.entries[i].bits = 0x0000;
        }
    }

    struct SecondLevelPageTable secondLevelPageTable;
    if (level == 2)
    {
        for (int j = 0; j < SECOND_LEVEL_TABLE_SIZE; j++)
        {
            struct FirstLevelPageTable childPageTable;
            for (int i = 0; i < SECOND_LEVEL_TABLE_SIZE; i++)
            {
                childPageTable.entries[i].bits = 0x0000;
            }
            secondLevelPageTable.tables[j] = childPageTable;
        }
    }

    enum PageReplacementAlgorithm pAlgo;

    if (strcmp(algo, "FIFO") == 0)
    {
        pAlgo = FIFO;
    }
    else if (strcmp(algo, "LRU") == 0)
    {
        pAlgo = LRU;
    }
    else if (strcmp(algo, "CLOCK") == 0)
    {
        pAlgo = CLOCK;
    }
    else
    {
        pAlgo = ECLOCK;
    }

    struct ClockList clockList;
    initializeList(&clockList);

    int k_lsb = (int)log2(fcount); // least significant k bits, to point frame no

    char line[256];
    char mode;
    unsigned int virtualAddress;
    unsigned int pageIndex;
    char value; // optional, w mode onşy

    while (fgets(line, sizeof(line), input) != NULL)
    {
        clock = clock + 1;

        if (sscanf(line, " %c %x 0x%hhx", &mode, &virtualAddress, &value) == 3 && mode == 'w')
        {
            pageIndex = virtualAddress >> 6;
            printf("Wrşte operation (mode: %c, virtual address: 0x%hhx, page index: %d, value 0x%hhx )\n", mode, virtualAddress, pageIndex, value);
        }
        else if (sscanf(line, " %c %x", &mode, &virtualAddress) == 2 && mode == 'r')
        {
            pageIndex = virtualAddress >> 6;
            printf("Read operation (mode: %c, virtual address: 0x%hhx, page index: %d)\n", mode, virtualAddress, pageIndex);
        }
        else
        {
            printf("Invalid line: %s", line);
        }

        int pageIndex1 = 0;
        // page table access
        struct FirstLevelPageTable *pageTable;
        if (level == 1)
        {
            pageTable = &firstLevelPageTable;
        }
        else
        {
            int pageIndex2 = pageIndex & 0x1f; // last 5 bits
            pageIndex1 = pageIndex >> 5;       // first 5 bits

            pageTable = &(secondLevelPageTable.tables[pageIndex1]);
            pageIndex = pageIndex2;
        }

        int pf = 0;
        // INVALID
        // printf("FIRST BIT OF ENTRY 0x%x: %d",pageTable->entries[pageIndex].bits,(pageTable->entries[pageIndex].bits >> 15));
        if (((int)pageTable->entries[pageIndex].bits) >> 15 == 0)
        {
            pf = 1;
            totalPageFault = totalPageFault + 1;
            // printf("Page fault for page %d\n", pageIndex);

            char buffer[64];

            if (level == 1)
            {
                fseek(disc, pageIndex * PAGE_SIZE, SEEK_SET);
            }
            else
            {
                fseek(disc, (pageIndex1 * SECOND_LEVEL_TABLE_SIZE + pageIndex) * PAGE_SIZE, SEEK_SET);
            }

            fread(buffer, PAGE_SIZE, 1, disc);

            int ramIndex = -1;

            for (int i = 0; i < fcount; ++i)
            {
                if (hasNonNullOrNot(ram.data[i].chars) == 0)
                {
                    printf("Empty space at ram index : %d \n", i);
                    ramIndex = i;
                    break;
                }
            }

            //  empty space exists
            if (ramIndex > -1)
            {
                for (int j = 0; j < PAGE_SIZE; ++j)
                {
                    ram.data[ramIndex].chars[j] = buffer[j];
                }

                pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | V_MASK;
                pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | R_MASK;
                // printf("Entry %d validated & referenced: 0x%x\n", pageIndex, pageTable->entries[pageIndex].bits);

                pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits + ramIndex;
                // printf("Entry value now: 0x%x\n", pageTable->entries[pageIndex].bits);
            }
            else
            {
                if (pAlgo == FIFO)
                {

                    // search for the page that first entered to ram
                    for (int i = 0; i < VIRTUAL_MEMORY_SIZE; i++)
                    {
                        unsigned int entry = 0x000;
                        if (level == 1)
                        {
                            entry = firstLevelPageTable.entries[i].bits;
                        }
                        else
                        {
                            entry = secondLevelPageTable.tables[(int)(i / SECOND_LEVEL_TABLE_SIZE)].entries[i % SECOND_LEVEL_TABLE_SIZE].bits;
                        }
                        printf("FOR PAGE %d ENTRY: 0x%x", i, entry);
                        // valid
                        if (entry >> 15 == 1)
                        {

                            unsigned int ram_pointer = entry & ((int)pow(2, k_lsb) - 1);

                            // found
                            if (ram_pointer == recent)
                            {

                                // modified, write to swap before removing page
                                if ((entry & M_MASK) >> 13 == 1)
                                {

                                    fseek(disc, i * PAGE_SIZE, SEEK_SET);

                                    for (int k = 0; k < PAGE_SIZE; k++)
                                    {
                                        unsigned char c = ram.data[ram_pointer].chars[k];
                                        fwrite(&c, sizeof(char), 1, disc);
                                    }
                                }
                                printf("Page %d removed from RAM Frame %d\n", i, recent);
                                if (level == 1)
                                {
                                    firstLevelPageTable.entries[i].bits = 0x0000;
                                }
                                else
                                {
                                    secondLevelPageTable.tables[(int)(i / SECOND_LEVEL_TABLE_SIZE)].entries[i % SECOND_LEVEL_TABLE_SIZE].bits = 0x0000;
                                }
                                break;
                            }
                        }
                    }
                    for (int j = 0; j < PAGE_SIZE; ++j)
                    {
                        ram.data[recent].chars[j] = buffer[j];
                    }

                    pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | V_MASK;
                    pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | R_MASK;
                    pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits + recent;
                    printf("Page %d put into frame %d\n", pageIndex, recent);
                    recent = (recent + 1) % fcount;
                }
                else if (pAlgo == LRU)
                {
                    unsigned int entry = 0x0000;
                    int min = 0;
                    int minClock = INT_MAX;
                    unsigned int ram_pointer;
                    if (level == 1)
                    {
                        for (int i = 0; i < VIRTUAL_MEMORY_SIZE; i++)
                        {
                            if (firstLevelPageTable.entries[i].bits >> 15 == 1 && firstLevelPageTable.entries[i].clock < minClock)
                            {
                                min = i;
                                minClock = firstLevelPageTable.entries[i].clock;
                                entry = firstLevelPageTable.entries[i].bits;
                            }
                        }

                        ram_pointer = entry & ((int)pow(2, k_lsb) - 1);
                        if ((entry & M_MASK) >> 13 == 1)
                        {
                            fseek(disc, min * PAGE_SIZE, SEEK_SET);
                            for (int k = 0; k < PAGE_SIZE; k++)
                            {
                                unsigned char c = ram.data[ram_pointer].chars[k];
                                fwrite(&c, sizeof(char), 1, disc);
                            }
                        }

                        firstLevelPageTable.entries[min].bits = 0x0000;
                    }
                    else
                    {
                        int pi1 = 0;
                        for (int i = 0; i < SECOND_LEVEL_TABLE_SIZE; i++)
                        {
                            for (int j = 0; j < SECOND_LEVEL_TABLE_SIZE; j++)
                            {
                                if (secondLevelPageTable.tables[i].entries[j].bits >> 15 == 1 && secondLevelPageTable.tables[i].entries[j].clock < minClock)
                                {
                                    pi1 = i;
                                    min = j;
                                    minClock = secondLevelPageTable.tables[i].entries[j].clock;
                                    entry = secondLevelPageTable.tables[i].entries[j].bits;
                                }
                            }
                        }

                        ram_pointer = entry & ((int)pow(2, k_lsb) - 1);
                        if ((secondLevelPageTable.tables[pi1].entries[min].bits & M_MASK) >> 13 == 1)
                        {
                            fseek(disc, (pi1 * SECOND_LEVEL_TABLE_SIZE + min) * PAGE_SIZE, SEEK_SET);
                            for (int k = 0; k < PAGE_SIZE; k++)
                            {
                                unsigned char c = ram.data[ram_pointer].chars[k];
                                fwrite(&c, sizeof(char), 1, disc);
                            }
                        }

                        secondLevelPageTable.tables[pi1].entries[min].bits = 0x000;
                    }

                    printf("Page %d removed from RAM Frame %d\n", min, ram_pointer);

                    for (int j = 0; j < PAGE_SIZE; ++j)
                    {
                        ram.data[ram_pointer].chars[j] = buffer[j];
                    }

                    pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | V_MASK;
                    pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | R_MASK;
                    pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits + ram_pointer;
                    printf("Page %d put into frame %d\n", pageIndex, ram_pointer);
                }
                else if (pAlgo == CLOCK)
                {
                    struct Node *current = clockList.head;
                    unsigned int pi = 0;
                    if (current != NULL)
                    {
                        int loop = 0;
                        int found = 0;
                        do
                        {
                            if (current == NULL)
                            {
                                current = clockList.head;
                            }

                            pi = current->data;
                            unsigned int entry = 0x0000;
                            if (level == 1)
                            {
                                entry = firstLevelPageTable.entries[pi].bits;
                            }
                            else
                            {
                                entry = secondLevelPageTable.tables[(int)(pi / SECOND_LEVEL_TABLE_SIZE)].entries[pi % SECOND_LEVEL_TABLE_SIZE].bits;
                            }

                            // check for valid, it supposed to be valid
                            // printf("PI %d CLOCK ENTRY : 0x%x \n",pi,entry);
                            if (entry >> 15 == 1)
                            {
                                // found
                                if ((entry & R_MASK) >> 14 == 0)
                                {
                                    found = 1;
                                    // remove from clockList
                                    removeFromList(&clockList, pi);
                                    unsigned int ram_pointer = entry & ((int)pow(2, k_lsb) - 1);

                                    // save to swap if modified
                                    if ((entry & M_MASK) >> 13 == 1)
                                    {

                                        fseek(disc, pi * PAGE_SIZE, SEEK_SET);

                                        for (int k = 0; k < PAGE_SIZE; k++)
                                        {
                                            unsigned char c = ram.data[ram_pointer].chars[k];
                                            fwrite(&c, sizeof(char), 1, disc);
                                        }
                                    }
                                    printf("Page %d removed from RAM Frame %d\n", pi, ram_pointer);

                                    // reset page table entry
                                    if (level == 1)
                                    {
                                        firstLevelPageTable.entries[pi].bits = 0x0000;
                                    }
                                    else
                                    {
                                        secondLevelPageTable.tables[(int)(pi / SECOND_LEVEL_TABLE_SIZE)].entries[pi % SECOND_LEVEL_TABLE_SIZE].bits = 0x0000;
                                    }

                                    for (int j = 0; j < PAGE_SIZE; ++j)
                                    {
                                        ram.data[ram_pointer].chars[j] = buffer[j];
                                    }

                                    // put corresponding page tabble entry to valid & referenced and ram index
                                    pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | V_MASK;
                                    pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | R_MASK;
                                    pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits + ram_pointer;
                                    printf("Page %d put into frame %d\n", (pageIndex1 * SECOND_LEVEL_TABLE_SIZE) + pageIndex, ram_pointer);
                                    break;
                                }
                                else // not found, set r bit to 0
                                {
                                    if (level == 1)
                                    {
                                        firstLevelPageTable.entries[pi].bits = entry & 0xBFFF;
                                    }
                                    else
                                    {
                                        secondLevelPageTable.tables[(int)(pi / SECOND_LEVEL_TABLE_SIZE)].entries[pi % SECOND_LEVEL_TABLE_SIZE].bits = entry & 0xBFFF;
                                    }
                                }
                            }

                            current = current->next;
                        } while (found == 0);
                    }
                }
                else
                {
                    struct Node *current = clockList.head;
                    unsigned int pi = 0;
                    int step = 1;
                    if (current != NULL)
                    {
                        int loop = 0;
                        int found = 0;
                        do
                        {
                            if (current == NULL)
                            {
                                current = clockList.head;
                                step = step + 1;
                            }

                            pi = current->data;
                            unsigned int entry = 0x0000;
                            if (level == 1)
                            {
                                entry = firstLevelPageTable.entries[pi].bits;
                            }
                            else
                            {
                                entry = secondLevelPageTable.tables[(int)(pi / SECOND_LEVEL_TABLE_SIZE)].entries[pi % SECOND_LEVEL_TABLE_SIZE].bits;
                            }

                            // check for valid, it supposed to be valid
                            // printf("PI %d CLOCK ENTRY : 0x%x \n",pi,entry);
                            if (entry >> 15 == 1)
                            {
                                // found
                                if (step == 1 && (entry & R_MASK) >> 14 == 0 && (entry & M_MASK) >> 13 == 0)
                                {
                                    found = 1;
                                    // remove from clockList
                                    removeFromList(&clockList, pi);
                                    unsigned int ram_pointer = entry & ((int)pow(2, k_lsb) - 1);

                                    // save to swap if modified
                                    if ((entry & M_MASK) >> 13 == 1)
                                    {

                                        fseek(disc, pi * PAGE_SIZE, SEEK_SET);

                                        for (int k = 0; k < PAGE_SIZE; k++)
                                        {
                                            unsigned char c = ram.data[ram_pointer].chars[k];
                                            fwrite(&c, sizeof(char), 1, disc);
                                        }
                                    }
                                    printf("Page %d removed from RAM Frame %d\n", pi, ram_pointer);

                                    // reset page table entry
                                    if (level == 1)
                                    {
                                        firstLevelPageTable.entries[pi].bits = 0x0000;
                                    }
                                    else
                                    {
                                        secondLevelPageTable.tables[(int)(pi / SECOND_LEVEL_TABLE_SIZE)].entries[pi % SECOND_LEVEL_TABLE_SIZE].bits = 0x0000;
                                    }

                                    for (int j = 0; j < PAGE_SIZE; ++j)
                                    {
                                        ram.data[ram_pointer].chars[j] = buffer[j];
                                    }

                                    // put corresponding page tabble entry to valid & referenced and ram index
                                    pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | V_MASK;
                                    pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | R_MASK;
                                    pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits + ram_pointer;
                                    printf("Page %d put into frame %d\n", (pageIndex1 * SECOND_LEVEL_TABLE_SIZE) + pageIndex, ram_pointer);
                                    break;
                                }

                                if (step == 2)
                                {
                                    if ((entry & R_MASK) >> 14 == 0 && (entry & M_MASK) >> 13 == 1)
                                    {
                                        found = 1;
                                        // remove from clockList
                                        removeFromList(&clockList, pi);
                                        unsigned int ram_pointer = entry & ((int)pow(2, k_lsb) - 1);

                                        // save to swap if modified
                                        if ((entry & M_MASK) >> 13 == 1)
                                        {

                                            fseek(disc, pi * PAGE_SIZE, SEEK_SET);

                                            for (int k = 0; k < PAGE_SIZE; k++)
                                            {
                                                unsigned char c = ram.data[ram_pointer].chars[k];
                                                fwrite(&c, sizeof(char), 1, disc);
                                            }
                                        }
                                        printf("Page %d removed from RAM Frame %d\n", pi, ram_pointer);

                                        // reset page table entry
                                        if (level == 1)
                                        {
                                            firstLevelPageTable.entries[pi].bits = 0x0000;
                                        }
                                        else
                                        {
                                            secondLevelPageTable.tables[(int)(pi / SECOND_LEVEL_TABLE_SIZE)].entries[pi % SECOND_LEVEL_TABLE_SIZE].bits = 0x0000;
                                        }

                                        for (int j = 0; j < PAGE_SIZE; ++j)
                                        {
                                            ram.data[ram_pointer].chars[j] = buffer[j];
                                        }

                                        // put corresponding page tabble entry to valid & referenced and ram index
                                        pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | V_MASK;
                                        pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | R_MASK;
                                        pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits + ram_pointer;
                                        printf("Page %d put into frame %d\n", (pageIndex1 * SECOND_LEVEL_TABLE_SIZE) + pageIndex, ram_pointer);
                                        break;
                                    }
                                    else // not found, set r bit to 0
                                    {
                                        if (level == 1)
                                        {
                                            firstLevelPageTable.entries[pi].bits = entry & 0xBFFF;
                                        }
                                        else
                                        {
                                            secondLevelPageTable.tables[(int)(pi / SECOND_LEVEL_TABLE_SIZE)].entries[pi % SECOND_LEVEL_TABLE_SIZE].bits = entry & 0xBFFF;
                                        }
                                    }
                                }

                                if (step == 3)
                                {
                                    if ((entry & R_MASK) >> 14 == 0 && (entry & M_MASK) >> 13 == 0)
                                    {
                                        found = 1;
                                        // remove from clockList
                                        removeFromList(&clockList, pi);
                                        unsigned int ram_pointer = entry & ((int)pow(2, k_lsb) - 1);

                                        // save to swap if modified
                                        if ((entry & M_MASK) >> 13 == 1)
                                        {

                                            fseek(disc, pi * PAGE_SIZE, SEEK_SET);

                                            for (int k = 0; k < PAGE_SIZE; k++)
                                            {
                                                unsigned char c = ram.data[ram_pointer].chars[k];
                                                fwrite(&c, sizeof(char), 1, disc);
                                            }
                                        }
                                        printf("Page %d removed from RAM Frame %d\n", pi, ram_pointer);

                                        // reset page table entry
                                        if (level == 1)
                                        {
                                            firstLevelPageTable.entries[pi].bits = 0x0000;
                                        }
                                        else
                                        {
                                            secondLevelPageTable.tables[(int)(pi / SECOND_LEVEL_TABLE_SIZE)].entries[pi % SECOND_LEVEL_TABLE_SIZE].bits = 0x0000;
                                        }

                                        for (int j = 0; j < PAGE_SIZE; ++j)
                                        {
                                            ram.data[ram_pointer].chars[j] = buffer[j];
                                        }

                                        // put corresponding page tabble entry to valid & referenced and ram index
                                        pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | V_MASK;
                                        pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | R_MASK;
                                        pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits + ram_pointer;
                                        printf("Page %d put into frame %d\n", (pageIndex1 * SECOND_LEVEL_TABLE_SIZE) + pageIndex, ram_pointer);
                                        break;
                                    }
                                }
                                if (step == 4)
                                {
                                    if ((entry & R_MASK) >> 14 == 0 && (entry & M_MASK) >> 13 == 1)
                                    {
                                        found = 1;
                                        // remove from clockList
                                        removeFromList(&clockList, pi);
                                        unsigned int ram_pointer = entry & ((int)pow(2, k_lsb) - 1);

                                        // save to swap if modified
                                        if ((entry & M_MASK) >> 13 == 1)
                                        {

                                            fseek(disc, pi * PAGE_SIZE, SEEK_SET);

                                            for (int k = 0; k < PAGE_SIZE; k++)
                                            {
                                                unsigned char c = ram.data[ram_pointer].chars[k];
                                                fwrite(&c, sizeof(char), 1, disc);
                                            }
                                        }
                                        printf("Page %d removed from RAM Frame %d\n", pi, ram_pointer);

                                        // reset page table entry
                                        if (level == 1)
                                        {
                                            firstLevelPageTable.entries[pi].bits = 0x0000;
                                        }
                                        else
                                        {
                                            secondLevelPageTable.tables[(int)(pi / SECOND_LEVEL_TABLE_SIZE)].entries[pi % SECOND_LEVEL_TABLE_SIZE].bits = 0x0000;
                                        }

                                        for (int j = 0; j < PAGE_SIZE; ++j)
                                        {
                                            ram.data[ram_pointer].chars[j] = buffer[j];
                                        }

                                        // put corresponding page tabble entry to valid & referenced and ram index
                                        pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | V_MASK;
                                        pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | R_MASK;
                                        pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits + ram_pointer;
                                        printf("Page %d put into frame %d\n", (pageIndex1 * SECOND_LEVEL_TABLE_SIZE) + pageIndex, ram_pointer);
                                        break;
                                    }
                                }
                            }
                            current = current->next;
                        } while (found == 0);
                    }
                    else
                    {
                        printf("Circular List Error, exiting...");
                        return 1;
                    }
                }
            }
        }

        pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | R_MASK;
        unsigned int ram_i = pageTable->entries[pageIndex].bits & ((int)pow(2, k_lsb) - 1);
        unsigned int offset = virtualAddress & 0x3F;
        char offsetValue = ram.data[ram_i].chars[offset];
        unsigned int pa = ram_i * PAGE_SIZE + offset;
        if (mode == 'r')
        {
            if (level == 1)
            {
                printf("Value 0x%hhx is read from page %d from frame %d \n", offsetValue, pageIndex, ram_i);
            }
            else
            {
                printf("Value 0x%hhx is read from (page %d - page %d) from frame %d \n", offsetValue, pageIndex1, pageIndex, ram_i);
            }
        }
        else
        {
            // write value
            ram.data[ram_i].chars[offset] = value;
            pageTable->entries[pageIndex].bits = pageTable->entries[pageIndex].bits | M_MASK;
            char newVal = ram.data[ram_i].chars[offset];
            printf("Entry %d modified: 0x%x\n", pageIndex, pageTable->entries[pageIndex].bits);

            if (level == 1)
            {
                printf("(Old value 0x%x) New value 0x%hhx is written to page %d to frame %d \n", offsetValue, newVal, pageIndex, ram_i);
            }
            else
            {
                printf("(Old value 0x%x) New value 0x%hhx is written to (page %d - page %d) to frame %d \n", offsetValue, newVal, pageIndex1, pageIndex, ram_i);
            }
        }

        if (pAlgo == LRU)
        {
            pageTable->entries[pageIndex].clock = clock;
        }

        if (pAlgo == CLOCK || pAlgo == ECLOCK)
        {
            if (level == 1)
            {
                addToTail(&clockList, pageIndex);
            }
            else
            {
                addToTail(&clockList, pageIndex1 * SECOND_LEVEL_TABLE_SIZE + pageIndex);
            }
        }

        // to do reminder: kısalt
        if (pf == 1)
        {
            if (level == 1)
            {

                fprintf(output, "0x%x 0x%x %s 0x%x 0x%x 0x%x %s\n", virtualAddress, pageIndex, "", offset, ram_i, pa, "pagefault");
            }
            else
            {
                fprintf(output, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x %s\n", virtualAddress, pageIndex1, pageIndex, offset, ram_i, pa, "pagefault");
            }
        }
        else
        {
            if (level == 1)
            {
                fprintf(output, "0x%x 0x%x %s 0x%x 0x%x 0x%x %s\n", virtualAddress, pageIndex, "", offset, ram_i, pa, "");
            }
            else
            {
                fprintf(output, "0x%x 0x%x 0x%x 0x%x 0x%x 0x%x %s\n", virtualAddress, pageIndex1, pageIndex, offset, ram_i, pa, "");
            }
        }
    }

    fprintf(output, "Page Fault No: %d", totalPageFault);

    for (int i = 0; i < VIRTUAL_MEMORY_SIZE; i++)
    {

        unsigned int entry;
        if (level == 1)
        {
            entry = firstLevelPageTable.entries[i].bits;
        }
        else
        {
            entry = secondLevelPageTable.tables[(int)(i / 32)].entries[i % 32].bits;
        }
        // printf("entry: 0x%x, first bit: %d\n",entry,(firstLevelPageTable.entries[i].bits >> 15));

        // if entry valid and in ram
        if (entry >> 15 != 0)
        {
            unsigned int ram_i = entry & ((int)pow(2, k_lsb) - 1);
            fseek(disc, i * PAGE_SIZE, SEEK_SET);

            for (int k = 0; k < PAGE_SIZE; k++)
            {
                unsigned char c = ram.data[ram_i].chars[k];
                // printf("wroting to file 0x%hhx\n",c);
                fwrite(&c, sizeof(char), 1, disc);
            }
        }
    }

    free(ram.data);
    ram.data = 0;
    ram.data = NULL;
    fclose(input);
    fclose(disc);
    fclose(output);

    return 0;
}
