#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <pthread.h>
#include "shared_defs.h"
#include <fcntl.h>
#include "queue.h"
#include <string.h>

#define MAX_CLIENTS 10
#define REQUEST_BUFFER_SIZE 256

// Global variables
mqd_t mq1, mq2;
pthread_mutex_t response_mutex[MAX_CLIENTS];
pthread_cond_t response_cond[MAX_CLIENTS];
char fname[256];
int vsize;
FILE *filePointers[MAX_CLIENTS];
int dlevel;
int clicount;
ResponseQueue responseQueues[MAX_CLIENTS];

Message parseRequestString(const char *requestString)
{
    Message message;
    memset(&message, 0, sizeof(Message));
    printf(requestString);

    char typeStr[20];
    if (sscanf(requestString, "%s", typeStr) < 1)
    {
        fprintf(stderr, "Error parsing request string: %s\n", requestString);
        exit(EXIT_FAILURE);
    }

    if (strcmp(typeStr, "GET") == 0)
    {
        message.messageType = GET_REQUEST;
        if (sscanf(requestString, "%*s %ld", &(message.key)) < 1)
        {
            fprintf(stderr, "Error parsing GET request string: %s\n", requestString);
        exit(EXIT_FAILURE);

        }
    }
    else if (strcmp(typeStr, "PUT") == 0)
    {
        char* v;
        message.messageType = PUT_REQUEST;
        int cc = sscanf(requestString, "%*s %ld %s", &(message.key), v);
        fprintf(stderr,"key: %d\n",message.key);
        printf("Value: %s\n", v);
        fprintf(stderr," count %d",cc);

        message.value = v;
        printf("Value: %s\n", message.value);
        if (cc<2)
        {
            fprintf(stderr, "Error parsing PUT request string: %s\n", requestString);
                    exit(EXIT_FAILURE);

        }
    }
    else if (strcmp(typeStr, "DEL") == 0)
    {
        message.messageType = DELETE_REQUEST;
        if (sscanf(requestString, "%*s %ld", &(message.key)) < 1)
        {
            fprintf(stderr, "Error parsing DELETE request string: %s\n", requestString);
                    exit(EXIT_FAILURE);


        }
    }
    else if (strcmp(typeStr, "QUITSERVER") == 0)
    {
        message.isServer = 1;
        message.quit = 1;
        message.messageType = -1;
    }
    else if (strcmp(typeStr, "DUMP") == 0)
    {
        message.messageType = DUMP;
        if (sscanf(requestString, "%*s %s", message.value) < 1)
        {
            fprintf(stderr, "Error parsing DUMP request string: %s\n", requestString);
                    exit(EXIT_FAILURE);


        }
    }
    else
    {
        fprintf(stderr, "Unknown request type in string: %s\n", requestString);
                exit(EXIT_FAILURE);


    }

    return message;
}

void *clientWorker(void *arg)
{
    int thread_id = *((int *)arg);

    while (1)
    {
        char requestBuffer[REQUEST_BUFFER_SIZE];

        if (fgets(requestBuffer, REQUEST_BUFFER_SIZE, filePointers[thread_id]) == NULL)
        {
            break;
        }

     /*    char *newline = strchr(requestBuffer, '\n');
        if (newline != NULL)
        {
            *newline = '\0';
        } */

        Message requestMessage = parseRequestString(requestBuffer);
        requestMessage.id = thread_id;

        if (mq_send(mq1, (const char *)&requestMessage, sizeof(Message), 0) == -1)
        {
            printf("Error sending request message to MQ1");
            exit(EXIT_FAILURE);
        }
        pthread_mutex_lock(&response_mutex[thread_id]);

        while (responseQueues[thread_id].front == NULL)
        {
            pthread_cond_wait(&response_cond[thread_id], &response_mutex[thread_id]);
        }

        Message responseMessage = dequeue(&responseQueues[thread_id]);

        pthread_mutex_unlock(&response_mutex[thread_id]);

        if (requestMessage.messageType == PUT_REQUEST)
        {
            free(requestMessage.value);
        }
    }

    pthread_exit(NULL);

}

void *frontend(void *arg)
{
    while (1)
    {
        Message responseMessage;
        if (mq_receive(mq2, (char *)&responseMessage, sizeof(Message), NULL) > 0)
        {
            // Extract the thread ID from the response message
            int thread_id = responseMessage.id;

            // Lock the mutex for the corresponding client thread
            pthread_mutex_lock(&response_mutex[thread_id]);

            // Enqueue the response message into the corresponding queue
            enqueue(&responseQueues[thread_id], responseMessage);

            // Signal or broadcast to wake up the corresponding client thread
            pthread_cond_signal(&response_cond[thread_id]);

            // Unlock the mutex for the corresponding client thread
            pthread_mutex_unlock(&response_mutex[thread_id]);
        }
    }

    pthread_exit(NULL);

}

int main(int argc, char *argv[])
{
    if (argc != 11)
    {
        printf("Usage: ./clientk -n <clicount> -f <fname> -s <vsize> -m <mqname> -d <dlevel>\n");
        exit(EXIT_FAILURE);
    }

    char mqname[256];

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
        snprintf(filename, sizeof(filename), "%s%d%s", fname, i + 1,".txt");

        filePointers[i] = fopen(filename, "r");
        if (filePointers[i] == NULL)
        {
            fprintf(stderr, "Failed to open file: %s\n", filename);
            exit(EXIT_FAILURE);
        }

        printf("File opened");
        printf(filename);
        printf("\n");
    }

    for (int i = 0; i < clicount; ++i)
    {
        pthread_mutex_init(&response_mutex[i], NULL);
        pthread_cond_init(&response_cond[i], NULL);
        ResponseQueue q;
        initQueue(&q);
        responseQueues[i] = q;

    }

    ResponseQueue responseQueues[MAX_CLIENTS];

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        initQueue(&responseQueues[i]);
    }

    pthread_t frontendThread;
    if (pthread_create(&frontendThread, NULL, frontend, NULL) != 0)
    {
        perror("Error creating frontend thread");
        exit(EXIT_FAILURE);
    }

    int thread_ids[clicount];
    pthread_t clientThreads[MAX_CLIENTS];
    for (int i = 0; i < clicount; ++i)
    {
        thread_ids[i] = i;
        if (pthread_create(&clientThreads[i], NULL, clientWorker, (void *)&thread_ids[i]) != 0)
        {
            perror("Error creating client thread");
            exit(EXIT_FAILURE);
        }
    }

    pthread_join(frontendThread, NULL);

    for (int i = 0; i < clicount; ++i)
    {
        pthread_join(clientThreads[i], NULL);
    }

    mq_close(mq1);
    mq_close(mq2);

    // Destroy condition variables
    for (int i = 0; i < clicount; ++i)
    {
        pthread_cond_destroy(&response_cond[i]);
    }

    return 0;
}
