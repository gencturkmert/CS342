#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define PAGE_SIZE 64
#define VIRTUAL_MEMORY_SIZE 1024

#define SECOND_LEVEL_SIZE 5
#define SECOND_LEVEL_TABLE_SIZE 32

int level;
char addrfile[100];
char swapfile[100];
int fcount;
char algo[10];
int tick;
char outfile[100];

char *ram;

// R+M+V + UNUSED BITS + K BITS
struct PageTableEntry
{
    char bits[16];
};

struct FirstLevelPageTable
{
    struct PageTableEntry entries[VIRTUAL_MEMORY_SIZE];
};

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
            printUsage();
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

    // allocate memory for simulated ram
    ram = (char *)malloc(fcount * PAGE_SIZE);
    if (ram == NULL)
    {
        printf("Error allocating memory for RAM");
        fclose(input);
        fclose(disc);
        fclose(output);
        return 1;
    }

    memset(ram, 0, fcount * PAGE_SIZE);

    struct FirstLevelPageTable *pageTable;
    if (level == 1)
    {
        for (int i = 0; i < VIRTUAL_MEMORY_SIZE; i++)
        {
            memset(firstLevelPageTable.entries[i].bits, '0', sizeof(firstLevelPageTable.entries[i].bits));
        }
    }

    if (level == 2)
    {
        printf("SECOND LEVEL NOT IMPLEMENTED");
    }

    int k_lsb = log2(fcount);

    free(ram);
    fclose(input);
    fclose(disc);
    fclose(output);

    return 0;
}
