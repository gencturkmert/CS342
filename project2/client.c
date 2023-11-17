#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <pthread.h>
#include "shared_defs.h"
#include <fcntl.h>

#define MAX_THREADS 10
#define REQUEST_BUFFER_SIZE 256

// Global variables
mqd_t mq1, mq2;
pthread_cond_t response_cond[MAX_THREADS];
char fname[256];
int vsize;
FILE *filePointers[MAX_CLIENTS];

void *clientWorker(void *arg)
{
    int thread_id = *((int *)arg);

    while (1)
    {
        char requestBuffer[REQUEST_BUFFER_SIZE];

        // Use thread-specific file
        if (fgets(requestBuffer, REQUEST_BUFFER_SIZE, filePointers[thread_id]) == NULL)
        {
            pthread_exit(NULL);
        }

        Message requestMessage;

        if (mq_send(mq1, (const char *)&requestMessage, sizeof(Message), 0) == -1)
        {
            perror("Error sending request message to MQ1");
            exit(EXIT_FAILURE);
        }

        pthread_cond_wait(&response_cond[thread_id], NULL);
    }

    pthread_exit(NULL);
}

void *frontend(void *arg)
{
    while (1)
    {
        // Receive messages from MQ2 and wake up the corresponding thread
        Message responseMessage;
        if (mq_receive(mq2, (char *)&responseMessage, sizeof(Message), NULL) > 0)
        {
            // Extract the thread ID from the response message
            int thread_id = /* Extract thread ID from the responseMessage */;

            // Signal or broadcast to wake up the corresponding thread
            pthread_cond_signal(&response_cond[thread_id]);
        }
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    // Check if the number of arguments is correct
    if (argc != 12)
    {
        printf("Usage: ./clientk -n <clicount> -f <fname> -s <vsize> -m <mqname> -d <dlevel>\n");
        exit(EXIT_FAILURE);
    }

    int clicount, vsize, dlevel;
    char mqname[256];

    // Parse command line arguments
    for (int i = 1; i < argc; i += 2)
    {
        if (i + 1 >= argc)
        {
            printf("Invalid number of arguments\n");
            exit(EXIT_FAILURE);
        }

        if (strcmp(argv[i], "-n") == 0)
        {
            clicount = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-f") == 0)
        {
            strncpy(fname, argv[i + 1], sizeof(fname));
        }
        else if (strcmp(argv[i], "-s") == 0)
        {
            vsize = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-m") == 0)
        {
            strncpy(mqname, argv[i + 1], sizeof(mqname));
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            dlevel = atoi(argv[i + 1]);
        }
        else
        {
            printf("Invalid arguments\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("clicount: %d\n", clicount);
    printf("fname: %s\n", fname);
    printf("vsize: %d\n", vsize);
    printf("mqname: %s\n", mqname);
    printf("dlevel: %d\n", dlevel);

    // Append 1 and 2 to mqname for MQ1 and MQ2
    pthread_cond_t response_cond[MAX_THREADS];
    for (int i = 0; i < clicount; ++i)
    {
        pthread_cond_init(&response_cond[i], NULL);
    }

    char mqname1[256], mqname2[256];

    snprintf(mqname1, sizeof(mqname1), "/%s1", mqname);
    snprintf(mqname2, sizeof(mqname2), "/%s2", mqname);
    // Open MQ1 and MQ2
    mq1 = mq_open(mqname1, O_WRONLY);
    mq2 = mq_open(mqname2, O_RDONLY);

    if (mq1 == (mqd_t)-1 || mq2 == (mqd_t)-1)
    {
        perror("Error opening message queues");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < clicount; ++i)
    {
        char filename[256];
        snprintf(filename, sizeof(filename), "%s%d", fname, i + 1);

        filePointers[i] = fopen(filename, "r");
        if (filePointers[i] == NULL)
        {
            fprintf(stderr, "Failed to open file: %s\n", filename);
            exit(EXIT_FAILURE);
        }
    }

    // Create frontend thread
    pthread_t frontendThread;
    if (pthread_create(&frontendThread, NULL, frontend, NULL) != 0)
    {
        perror("Error creating frontend thread");
        exit(EXIT_FAILURE);
    }

    // Create client threads
    pthread_t clientThreads[MAX_THREADS];
    for (int i = 0; i < clicount; ++i)
    {
        if (pthread_create(&clientThreads[i], NULL, clientWorker, (void *)i) != 0)
        {
            perror("Error creating client thread");
            exit(EXIT_FAILURE);
        }
    }

    // Join frontend thread
    pthread_join(frontendThread, NULL);

    // Join client threads
    for (int i = 0; i < clicount; ++i)
    {
        pthread_join(clientThreads[i], NULL);
    }

    // Close MQ1 and MQ2
    mq_close(mq1);
    mq_close(mq2);

    // Destroy condition variables
    for (int i = 0; i < clicount; ++i)
    {
        pthread_cond_destroy(&response_cond[i]);
    }

    return 0;
}
