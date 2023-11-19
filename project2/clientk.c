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
bool quit = false;
pthread_cond_t quit_cond = PTHREAD_COND_INITIALIZER;

Message parseRequestString(const char *requestString)
{
    Message message;
    memset(&message, 0, sizeof(Message));
    message.isServer = 0;


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
        char *v = malloc(vsize);
        message.messageType = PUT_REQUEST;
        int cc = sscanf(requestString, "%*s %ld %s", &(message.key), v);
        if (cc < 2)
        {
            fprintf(stderr, "Error parsing PUT request string: %s\n", requestString);
            exit(EXIT_FAILURE);
        }
        strncpy(message.value,v,sizeof(message.value)-1);

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
        message.isServer = 0;
        message.messageType = QUITSERVER;

    }else if(strcmp(typeStr,"QUIT") == 0) {
        message.messageType = QUIT;
    }
    else if (strcmp(typeStr, "DUMP") == 0)
    {
        message.messageType = DUMP;
        char *v = malloc(vsize);
        int cc = sscanf(requestString, "%*s %s", v);
        if (cc < 1)
        {
            fprintf(stderr, "Error parsing DUMP request string: %s\n", requestString);
            exit(EXIT_FAILURE);
        }
        strncpy(message.value,v,vsize);
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

    while (!quit)
    {
        char requestBuffer[REQUEST_BUFFER_SIZE];

        if (fgets(requestBuffer, REQUEST_BUFFER_SIZE, filePointers[thread_id]) == NULL)
        {
            break;
        }

        Message requestMessage = parseRequestString(requestBuffer);

         if(requestMessage.messageType == QUIT) {
            quit = true;
            pthread_cond_signal(&quit_cond);
            break;
        }

        requestMessage.id = thread_id;
        if(dlevel == 1) {
            printf("Message %s sent for client thread %d \n", requestBuffer,thread_id);
        }

        if (mq_send(mq1, (const char *)&requestMessage, sizeof(Message), 0) == -1)
        {
            printf("Error sending request message to MQ1");
            exit(EXIT_FAILURE);
        }

        if(requestMessage.messageType == QUITSERVER) {
            quit = true;
            pthread_cond_signal(&quit_cond);
            pthread_exit(NULL);
        }
        pthread_mutex_lock(&response_mutex[thread_id]);

        while (responseQueues[thread_id].front == NULL)
        {
       
            pthread_cond_wait(&response_cond[thread_id], &response_mutex[thread_id]);
        }

        Message responseMessage = dequeue(&responseQueues[thread_id]);

        if(dlevel ==1 ) {
            if(responseMessage.success) {
                printf("Request %s returned succesfully \n",requestBuffer);
            }else{
                printf("Request %s failed\n",requestBuffer);
            }
            if(responseMessage.messageType == GET_REQUEST) {
                printf("Value %s got from server for key %d\n",responseMessage.value,responseMessage.key);
            }
        }

        pthread_mutex_unlock(&response_mutex[thread_id]);

      
    }

    pthread_exit(NULL);
}

void *frontend(void *arg)
{
    while (!quit)
    {
        Message responseMessage;
        if (mq_receive(mq2, (char *)&responseMessage, sizeof(Message), NULL) > 0)
        {
            int thread_id = responseMessage.id;

            pthread_mutex_lock(&response_mutex[thread_id]);

            enqueue(&responseQueues[thread_id], responseMessage);

            pthread_cond_signal(&response_cond[thread_id]);

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

if(dlevel == 1) {
    printf("clicount: %d\n", clicount);
    printf("fname: %s\n", fname);
    printf("vsize: %d\n", vsize);
    printf("mqname: %s\n", mqname);
    printf("dlevel: %d\n", dlevel);
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

    if(clicount == 0) {

        while(!quit) {

            char input[256];

            printf("Enter a request: ");
            
            fgets(input, sizeof(input), stdin);

            Message message = parseRequestString(input);

            if(message.messageType == QUIT) {
                quit = true;
                pthread_cond_signal(&quit_cond);
                break;
            }

            message.id = 0;
            if(dlevel == 1) {
                printf("Message %s sent \n", input); 
            }

            if (mq_send(mq1, (const char *)&message, sizeof(Message), 0) == -1)
            {
                printf("Error sending request message to MQ1");
                exit(EXIT_FAILURE);
            }

            if(message.messageType == QUITSERVER) {
                quit = true;
                pthread_cond_signal(&quit_cond);
                break;
            }


            bool loop = false;
            Message responseMessage;
            while (!loop)
            {
                if (mq_receive(mq2, (char *)&responseMessage, sizeof(Message), NULL) > 0)
                {
                    loop = true;
                }
            }



            if(dlevel ==1 ) {
                if(responseMessage.success) {
                    printf("Request %s returned succesfully \n",input);
                }else{
                    printf("Request %s failed\n",input);
                }
                if(responseMessage.messageType == GET_REQUEST) {
                    printf("Value %s got from server for key %d\n",responseMessage.value,responseMessage.key);
                }
            }

        }

    }else{

        for (int i = 0; i < clicount; ++i)
        {
            char filename[256];
            snprintf(filename, sizeof(filename), "%s%d%s", fname, i + 1, ".txt");

            filePointers[i] = fopen(filename, "r");
            if (filePointers[i] == NULL)
            {
                fprintf(stderr, "Failed to open file: %s\n", filename);
                exit(EXIT_FAILURE);
            }


        }
       

        for (int i = 0; i < clicount; ++i)
        {
            pthread_mutex_init(&response_mutex[i], NULL);
            pthread_cond_init(&response_cond[i], NULL);
        }

        for (int i = 0; i < clicount; ++i)
        {
            initQueue(&responseQueues[i]);
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

        pthread_t frontendThread;
        if (pthread_create(&frontendThread, NULL, frontend, NULL) != 0)
        {
            perror("Error creating frontend thread");
            exit(EXIT_FAILURE);
        }

        pthread_mutex_t quit_mutex = PTHREAD_MUTEX_INITIALIZER;
        while(!quit) {
            pthread_cond_wait(&quit_cond,&quit_mutex);
        }

           // Destroy condition variables
        for (int i = 0; i < clicount; ++i)
        {
            pthread_cond_destroy(&response_cond[i]);
        }
    }   

    pthread_cond_destroy(&quit_cond);
    mq_close(mq1);
    mq_close(mq2);
    printf("CLIENT EXIT");

 

    return 0;
}
