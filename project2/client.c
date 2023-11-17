#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <pthread.h>
#include "shared_defs.h"

#define MAX_THREADS 10
#define REQUEST_BUFFER_SIZE 256

// Global variables
mqd_t mq1, mq2;
pthread_t clientThreads[MAX_THREADS];
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t file_cond = PTHREAD_COND_INITIALIZER;
int current_line = 0;
char fname[256];

// Function prototypes
void *clientWorker(void *arg);

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
    char mqname1[256], mqname2[256];
    snprintf(mqname1, sizeof(mqname1), "%s1", mqname);
    snprintf(mqname2, sizeof(mqname2), "%s2", mqname);

    // Open MQ1 and MQ2
    mq1 = mq_open(mqname1, O_WRONLY);
    mq2 = mq_open(mqname2, O_RDONLY);

    if (mq1 == (mqd_t)-1 || mq2 == (mqd_t)-1)
    {
        perror("Error opening message queues");
        exit(EXIT_FAILURE);
    }

    // Initialize and create client threads
    for (int i = 0; i < clicount; ++i)
    {
        if (pthread_create(&clientThreads[i], NULL, clientWorker, NULL) != 0)
        {
            perror("Error creating client thread");
            exit(EXIT_FAILURE);
        }
    }

    // Join client threads
    for (int i = 0; i < clicount; ++i)
    {
        pthread_join(clientThreads[i], NULL);
    }

    // Close MQ1 and MQ2
    mq_close(mq1);
    mq_close(mq2);

    return 0;
}

void *clientWorker(void *arg)
{
    while (1)
    {
        // Lock the file mutex to read from the file
        pthread_mutex_lock(&file_mutex);

        // Read the next line from the file
        char requestBuffer[REQUEST_BUFFER_SIZE];
        if (fgets(requestBuffer, REQUEST_BUFFER_SIZE, fopen(fname, "r")) == NULL)
        {
            // End of file reached, release the mutex and exit the thread
            pthread_mutex_unlock(&file_mutex);
            break;
        }

        // Increment the current line counter
        current_line++;

        // Unlock the file mutex
        pthread_mutex_unlock(&file_mutex);

        // Create a request message
        Message requestMessage;
        requestMessage.isServer = false;
        requestMessage.quit = false;
        requestMessage.messageType = PUT_REQUEST; // Modify based on your requirements
        requestMessage.success = false;
        requestMessage.keySize = sizeof(long int);    // Modify based on your requirements
        requestMessage.valueSize = svsize;            // Modify based on your requirements
        requestMessage.key = 12345;                   // Modify based on your requirements
        requestMessage.value = strdup(requestBuffer); // Modify based on your requirements

        // Send the request message to MQ1
        if (mq_send(mq1, (const char *)&requestMessage, sizeof(Message), 0) == -1)
        {
            perror("Error sending request message to MQ1");
            exit(EXIT_FAILURE);
        }

        // Wait for the response on MQ2 (You need to implement this part based on your design)
        // You may need to use a separate function to handle the response.

        // ...

        // Signal or broadcast to wake up the main thread
        pthread_cond_signal(&file_cond);
    }

    pthread_exit(NULL);
}
