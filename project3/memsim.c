#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define PAGE_SIZE 64
#define VIRTUAL_MEMORY_SIZE 1024

#define SECOND_LEVEL_TABLE_SIZE 32

#define V_MASK 0x8000
#define R_MASK 0x4000
#define M_MASK 0x2000

// V+R+M+ UNUSED BITS + K BITS
// V = 0 is invalid
struct PageTableEntry
{
    unsigned short int bits;
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
    printf("algo: %s\n", algo);
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

    int k_lsb = (int)log2(fcount); // least significant k bits, to point frame no

    char line[256];
    char mode;
    unsigned int virtualAddress;
    unsigned int pageIndex;
    char value; // optional, w mode onşy

    while (fgets(line, sizeof(line), input) != NULL)
    {

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
        //printf("FIRST BIT OF ENTRY 0x%x: %d",pageTable->entries[pageIndex].bits,(pageTable->entries[pageIndex].bits >> 15));
        if (((int)pageTable->entries[pageIndex].bits) >> 15 == 0)
        {
            pf = 1;
            totalPageFault = totalPageFault + 1;
            printf("Page fault for page %d\n", pageIndex);

            char buffer[64];

            fseek(disc, pageIndex * PAGE_SIZE, SEEK_SET);

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
                //printf("Entry value now: 0x%x\n", pageTable->entries[pageIndex].bits);
            }
            else
            {
                // THERE IS NO EMPTY SPACE, USE ALGO
                printf("EMPTY SPACE NOT IMPLEMENTED YET\n");
            }
        }

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
    if (level == 1)
    {

        for (int i = 0; i < VIRTUAL_MEMORY_SIZE; i++)
        {

            unsigned int entry = firstLevelPageTable.entries[i].bits;
           // printf("entry: 0x%x, first bit: %d\n",entry,(firstLevelPageTable.entries[i].bits >> 15));

            // if entry valid and in ram
            if (((int)firstLevelPageTable.entries[i].bits) >> 15 != 0)
            {
                unsigned int ram_i = entry & ((int)pow(2, k_lsb) - 1);
                fseek(disc, i * PAGE_SIZE, SEEK_SET);

                for(int k = 0; k<PAGE_SIZE;k++) {
                    unsigned char c = ram.data[ram_i].chars[k];
                    //printf("wroting to file 0x%hhx\n",c);
                    fwrite(&c,sizeof(char),1, disc);

                }
            }
        }
    }
    else
    {
        for (int i = 0; i < SECOND_LEVEL_TABLE_SIZE; i++)
        {
            for (int j = 0; j < SECOND_LEVEL_TABLE_SIZE; j++)
            {
                unsigned int entry = secondLevelPageTable.tables[i].entries[j].bits;
                if ((entry & V_MASK) == 0x8000)
                {
                    unsigned int ram_i = entry & ((int)pow(2, k_lsb) - 1);
                    fseek(disc, (i*SECOND_LEVEL_TABLE_SIZE + j )* PAGE_SIZE, SEEK_SET);
                   
                   for(int k = 0; k<PAGE_SIZE;k++) {
                        unsigned char c = ram.data[ram_i].chars[k];
                        //printf("wroting to file 0x%hhx\n",c);
                        fwrite(&c,sizeof(char),1, disc);

                    }
                }
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
