#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>

// Global Variables
int serverk;
int ddcount;
char ffname[256];
int ttcount;
int svsize;
char mmqname[256];

int main(int argc, char *argv[])
{
    // Check if the number of arguments is correct
    if (argc != 13)
    {
        printf("Usage: ./your_program -serverk <value> -ddcount <value> -ffname <value> -ttcount <value> -svsize <value> -mmqname <value>\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i += 2)
    {
        if (i + 1 >= argc)
        {
            printf("Invalid number of arguments\n");
            exit(1);
        }

        if (strcmp(argv[i], "-serverk") == 0)
        {
            serverk = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-ddcount") == 0)
        {
            ddcount = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-ffname") == 0)
        {
            strncpy(ffname, argv[i + 1], sizeof(ffname));
        }
        else if (strcmp(argv[i], "-ttcount") == 0)
        {
            ttcount = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-svsize") == 0)
        {
            svsize = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-mmqname") == 0)
        {
            strncpy(mmqname, argv[i + 1], sizeof(mmqname));
        }
        else
        {
            printf("Invalid arguments\n");
            exit(1);
        }
    }

    printf("serverk: %d\n", serverk);
    printf("ddcount: %d\n", ddcount);
    printf("ffname: %s\n", ffname);
    printf("ttcount: %d\n", ttcount);
    printf("svsize: %d\n", svsize);
    printf("mmqname: %s\n", mmqname);

    mqd_t mq1, mq2;
    char mqname1[256], mqname2[256];

    snprintf(mqname1, sizeof(mqname1), "/%s1", mmqname);
    snprintf(mqname2, sizeof(mqname2), "/%s2", mmqname);

    // Open the message queues
    mq1 = mq_open(mqname1, O_CREAT | O_RDWR, 0666, NULL);
    mq2 = mq_open(mqname2, O_CREAT | O_RDWR, 0666, NULL);

    if (mq1 == (mqd_t)-1 || mq2 == (mqd_t)-1)
    {
        perror("Error opening message queues");
        exit(EXIT_FAILURE);
    }

    // Your program logic goes here

    // Close the message queues
    mq_close(mq1);
    mq_close(mq2);
    return 0;
}