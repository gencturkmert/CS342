#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define PAGE_SIZE 64
#define VIRTUAL_MEMORY_SIZE 1024

#define SECOND_LEVEL_SIZE 5
#define SECOND_LEVEL_TABLE_SIZE 32

#define V_MASK 0x80
#define R_MASK 0x40
#define M_MASK 0x20

// V+R+M+ UNUSED BITS + K BITS
// V = 0 is invalid
struct PageTableEntry
{
    unsigned short int = 0x00;
};

struct FirstLevelPageTable
{
    struct PageTableEntry entries[VIRTUAL_MEMORY_SIZE];
};
typedef struct
{
    char chars[PAGE_SIZE];
} Data;

typedef struct
{
    size_t size;
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

    for (size_t i = 0; i < fcount; ++i)
    {
        memset(ram.data[i].chars, '\0', PAGE_SIZE);
    }

    struct FirstLevelPageTable pageTable;
    if (level == 1)
    {
        // for (int i = 0; i < VIRTUAL_MEMORY_SIZE; i++)
        // {
        //     memset(pageTable.entries[i].bits, '0', sizeof(pageTable.entries[i].bits));
        // }
    }

    if (level == 2)
    {
        printf("SECOND LEVEL NOT IMPLEMENTED");
        return 1;
    }

    int k_lsb = (int)log2(fcount);

    char line[256];
    char mode;
    unsigned int virtualAddress;
    unsigned int pageIndex;
    unsigned int pageIndex2 = 0;
    unsigned int value; // optional, w mode onşy

    while (fgets(line, sizeof(line), input) != NULL)
    {

        if (sscanf(line, " %c %x %x", &mode, &virtualAddress, &value) == 3 && mode == 'w')
        {
            pageIndex = virtualAddress >> 6;
            printf("Wrşte operation (mode: %c, virtual address: 0x%x, page index: %d, value 0x%x )\n", mode, virtualAddress, pageIndex, value);
        }
        else if (sscanf(line, " %c %x", &mode, &virtualAddress) == 2 && mode == 'r')
        {
            pageIndex = virtualAddress >> 6;
            printf("Read operation (mode: %c, virtual address: 0x%x, page index: %d)\n", mode, virtualAddress, pageIndex);
        }
        else
        {
            printf("Invalid line: %s", line);
        }

        // page table access
        if (level == 1)
        {
            bool pf = false;
            // INVALID
            if (pageTable.entries[pageIndex].bits & V_MASK == 0)
            {
                pf = true;
                totalPageFault = totalPageFault + 1;
                printf("Page fault for page %d\n", pageIndex);

                char buffer[64];

                fseek(disc, pageIndex * PAGE_SIZE, SEEK_SET);

                fread(buffer, PAGE_SIZE, 1, disc);

                int ramIndex = -1;
                for (size_t i = 0; i < ram->size; ++i)
                {
                    if (memcmp(ram.data[i].chars, "\0", PAGE_SIZE) == 0)
                    {
                        ramIndex = i;
                        break;
                    }
                }

                // There is empty space
                if (ramIndex > 0)
                {
                    for (int j = 0; j < PAGE_SIZE; ++j)
                    {
                        ram.data[ramIndex].chars[j] = buffer[j];
                    }

                    pageTable.entries[pageIndex].bits == pageTable.entries[pageIndex].bits & 0x8000;

                    for (int i = k_lsb - 1; i >= 0; i--)
                    {
                        pageTable.entries[pageIndex].bits = pageTable.entries[pageIndex].bits + k_lsb;
                    }
                }
                else
                {
                    // THERE IS NO EMPTY SPACE, USE ALGO
                    printf("ALGORITHM NOT IMPLEMENTED YET");
                }
            }

            unsigned int ram_i = pageTable.entries[pageIndex].bits & ((int)pow(2, k_lsb) - 1);
            unsigned int offset = virtualAddress & 0x3F;
            unsigned int offsetValue = ram.data[ram_i].chars[offset];
            unsigned int pa = ram_i * PAGE_SIZE + offset;

            if (pf)
            {
                fprintf(output, "%x %x %s %x %x %x %s\n", virtualAddress, pageIndex, "", offset, ram_i, pa, "");
            }
            else
            {
                fprintf(output, "%x %x %s %x %x %x %s\n", virtualAddress, pageIndex, "", offset, ram_i, pa, "pagefault");
            }
        }
        else
        {
            printf("SECOND LEVEL NOT IMPLEMENTED");
            return 1;
        }
    }

    free(ram->data);
    ram->size = 0;
    ram->data = NULL;
    fclose(input);
    fclose(disc);
    fclose(output);

    return 0;
}
