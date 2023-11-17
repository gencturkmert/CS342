#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include "shared_defs.h"
#include <fcntl.h>
#include <pthread.h>

#define MAX_FILES 5

int nclicount;
char ffname[256];
int svsize;
char mmqname[256];
int ddlevel;

mqd_t mq1, mq2;

int main(int argc, char *argv[])
{
    // Check if the number of arguments is correct
    if (argc != 12)
    {
        printf("Usage: ./clientk -n <clicount> -f <fname> -s <vsize> -m <mqname> -d <dlevel>\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i += 2)
    {
        if (i + 1 >= argc)
        {
            printf("Invalid number of arguments\n");
            exit(EXIT_FAILURE);
        }

        if (strcmp(argv[i], "-n") == 0)
        {
            nclicount = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-f") == 0)
        {
            strncpy(ffname, argv[i + 1], sizeof(ffname));
        }
        else if (strcmp(argv[i], "-s") == 0)
        {
            svsize = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-m") == 0)
        {
            strncpy(mmqname, argv[i + 1], sizeof(mmqname));
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            ddlevel = atoi(argv[i + 1]);
        }
        else
        {
            printf("Invalid arguments\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("clicount: %d\n", nclicount);
    printf("fname: %s\n", ffname);
    printf("vsize: %d\n", svsize);
    printf("mqname: %s\n", mmqname);
    printf("dlevel: %d\n", ddlevel);

    char mqname1[256], mqname2[256];

    snprintf(mqname1, sizeof(mqname1), "/%s1", mmqname);
    snprintf(mqname2, sizeof(mqname2), "/%s2", mmqname);

    mq1 = mq_open(mqname1, O_CREAT | O_RDWR, 0666, NULL);
    mq2 = mq_open(mqname2, O_CREAT | O_RDWR, 0666, NULL);

    if (mq1 == (mqd_t)-1 || mq2 == (mqd_t)-1)
    {
        perror("Error opening message queues");
        exit(EXIT_FAILURE);
    }

    // TODO: Add your client logic here

    // Close the message queues
    mq_close(mq1);
    mq_close(mq2);

    return 0;
}
