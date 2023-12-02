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
}

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
    if (access(swapfile, F_OK) == -1)
    {
        disc = fopen(swapfile, "a+b"); // Open in binary write mode

        if (disc == NULL)
        {
            printf("Error creating swapfile");
            exit(1);
        }

        // Initialize to all zeros
        char zeroBuffer[PAGE_SIZE] = {0};

        for (int i = 0; i < VIRTUAL_MEMORY_SIZE; ++i)
        {
            fwrite(zeroBuffer, sizeof(char), PAGE_SIZE, disc);
        }
    }
    else
    {
        disc = fopen(swapfile, "a+b"); // a+ for read and write, b for binary
        if (disc == NULL)
        {
            printf("Error opening swapfile");
            fclose(input);
            return 1;
        }
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
            secondLevelPageTable.tables[i] = childPageTable;
        }
    }

    int k_lsb = (int)log2(fcount);

    char line[256];
    char mode;
    unsigned int virtualAddress;
    unsigned int pageIndex;
    char value; // optional, w mode onşy

    while (fgets(line, sizeof(line), input) != NULL)
    {

        if (sscanf(line, " %c %x %x", &mode, &virtualAddress, &value) == 3 && mode == 'w')
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

        struct FirstLevelPageTable pageTable;
        int pageIndex1 = 0;
        // page table access
        if (level == 1)
        {
            pageTable = firstLevelPageTable;
        }
        else
        {
            int pageIndex2 = pageIndex & 0x1f; // last 5 bits
            pageIndex1 = pageIndex >> 5;       // first 5 bits

            pageTable = secondLevelPageTable.tables[pageIndex1];
            pageIndex = pageIndex2;
        }

        int pf = 0;
        // INVALID
        if (((int)pageTable.entries[pageIndex].bits & V_MASK) >> 15 == 0)
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

            // There is empty space
            if (ramIndex > -1)
            {
                for (int j = 0; j < PAGE_SIZE; ++j)
                {
                    ram.data[ramIndex].chars[j] = buffer[j];
                }

                pageTable.entries[pageIndex].bits = pageTable.entries[pageIndex].bits | V_MASK;
                pageTable.entries[pageIndex].bits = pageTable.entries[pageIndex].bits | R_MASK;
                printf("Entry %d validated & referenced: 0x%x\n", pageIndex, pageTable.entries[pageIndex].bits);

                pageTable.entries[pageIndex].bits = pageTable.entries[pageIndex].bits + ramIndex;
                printf("Entry value now: 0x%x\n", pageTable.entries[pageIndex].bits);
            }
            else
            {
                // THERE IS NO EMPTY SPACE, USE ALGO
                printf("EMPTY SPACE NOT IMPLEMENTED YET\n");
            }
        }

        unsigned int ram_i = pageTable.entries[pageIndex].bits & ((int)pow(2, k_lsb) - 1);
        unsigned int offset = virtualAddress & 0x3F;
        unsigned int offsetValue = ram.data[ram_i].chars[offset];
        unsigned int pa = ram_i * PAGE_SIZE + offset;
        if (mode == 'r')
        {
            if (level == 1)
            {
                printf("Value 0x%x is read from page %d from frame %d \n", offsetValue, pageIndex, ram_i);
            }
            else
            {
                printf("Value 0x%x is read from (page %d - page %d) from frame %d \n", offsetValue, pageIndex1, pageIndex, ram_i);
            }
        }
        else
        {
            // write value
            ram.data[ram_i].charts[offset] = value;
            pageTable.entries[pageIndex].bits = pageTable.entries[pageIndex].bits | M_MASK;
            printf("Entry %d modified: 0x%x\n", pageIndex, pageTable.entries[pageIndex].bits);

            if (level == 1)
            {
                printf("(Old value 0x%x) New value 0x%x is written to page %d to frame %d \n", offsetValue, value, pageIndex, ram_i);
            }
            else
            {
                printf("(Old value 0x%x) New value 0x%x is written to (page %d - page %d) to frame %d \n", offsetValue, value, pageIndex1, pageIndex, ram_i);
            }
        }

        if (pf == 1)
        {
            fprintf(output, "0x%x 0x%x %s 0x%x 0x%x 0x%x %s\n", virtualAddress, pageIndex, "", offset, ram_i, pa, "pagefault");
        }
        else
        {
            fprintf(output, "0x%x 0x%x %s 0x%x 0x%x 0x%x %s\n", virtualAddress, pageIndex, "", offset, ram_i, pa, "");
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
