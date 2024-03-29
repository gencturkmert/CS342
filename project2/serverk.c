#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include "shared_defs.h"
#include "hash_table.h"

#define BUFFER_SIZE 128
#define MAX_FILES 5

int dcount;
char fname[256];
int tcount;
int vsize;
char mqname[256];

FILE *filePointers[MAX_FILES];
pthread_mutex_t file_mutex[MAX_FILES];
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mq1_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t mq1_full = PTHREAD_COND_INITIALIZER;

Message buffer[BUFFER_SIZE];

int front = 0;
int rear = 0;
int buffer_size = 0;

int deletion_marker = 0;
bool should_exit = false;
pthread_cond_t quit_cond = PTHREAD_COND_INITIALIZER;

HashTable globalHashTables[MAX_FILES];
pthread_mutex_t quit_mutex = PTHREAD_MUTEX_INITIALIZER;

mqd_t mq1, mq2;

void *worker(void *arg)
{
    int thread_id = *((int *)arg);

    while (!should_exit)
    {
        pthread_mutex_lock(&buffer_mutex);

        while (buffer_size == 0)
        {
            printf("Worker thread %d waits for signal\n", thread_id);

            if (should_exit)
            {
                pthread_exit(NULL);
            }

            pthread_cond_wait(&mq1_full, &buffer_mutex);
        }

        MessageType messageType = buffer[front].messageType;
        long int key = buffer[front].key;
        char *value = (char *)malloc(vsize);
        strncpy(value, buffer[front].value, vsize);
        int id = buffer[front].id;

        printf("Worker thread %d received request for %d\n", thread_id, messageType);
        printf("Key %d\n", key);
        printf("Value %s\n", value);
        printf("Client ID: %d\n", id);

        int file_index = (key % dcount);

        front = (front + 1) % BUFFER_SIZE;
        buffer_size--;

        pthread_mutex_unlock(&buffer_mutex);

        pthread_mutex_lock(&file_mutex[file_index]);

        switch (messageType)
        {
        case PUT_REQUEST:
        {
            size_t offset = getOffsetForKey(key, &globalHashTables[file_index]);
            FILE *file = filePointers[file_index];

            if (offset != -1)
            {

                fseek(file, offset - sizeof(long int), SEEK_SET);
                fwrite(&deletion_marker, sizeof(long int), 1, file);
                updateHashTable(&globalHashTables[file_index], key, -1);
            }
            /*  {
                 fseek(file,offset,SEEK_SET);
                 if (strlen(value) < vsize)
                 {
                     fwrite(value, sizeof(char), strlen(value), file);
                     size_t remainingBytes = vsize - strlen(value);
                     char nullBytes[remainingBytes];
                     memset(nullBytes, 0, remainingBytes);
                     fwrite(nullBytes, sizeof(char), remainingBytes, file);
                 }
                 else
                 {
                     fwrite(value, sizeof(char), vsize, file);
                 }
             } */
            // else
            //{
            fseek(file, 0, SEEK_END);
            fwrite(&key, sizeof(long int), 1, file);
            if (strlen(value) < vsize)
            {
                fwrite(value, sizeof(char), strlen(value), file);
                size_t remainingBytes = vsize - strlen(value);
                char nullBytes[remainingBytes];
                memset(nullBytes, 0, remainingBytes);
                fwrite(nullBytes, sizeof(char), remainingBytes, file);
            }
            else
            {
                fwrite(value, sizeof(char), vsize, file);
            }

            size_t newOffset = ftell(file) - vsize;
            updateHashTable(&globalHashTables[file_index], key, newOffset);
            // }

            Message responseMessage;
            memset(&responseMessage, 0, sizeof(Message));
            responseMessage.isServer = true;
            responseMessage.messageType = PUT_REQUEST;
            responseMessage.success = true;
            responseMessage.valueSize = 0;
            responseMessage.key = key;
            strncpy(responseMessage.value, "", vsize);
            responseMessage.id = id;

            if (mq_send(mq2, (const char *)&responseMessage, sizeof(Message), 0) == -1)
            {
                printf("Error sending PUT_RESPONSE message to mq2");
                exit(EXIT_FAILURE);
            }

            break;
        }

        case DELETE_REQUEST:
        {
            size_t offset = getOffsetForKey(key, &globalHashTables[file_index]);

            if (offset != -1)
            {
                FILE *file = filePointers[file_index];
                fseek(file, offset - sizeof(long int), SEEK_SET);
                fwrite(&deletion_marker, sizeof(long int), 1, file);
                updateHashTable(&globalHashTables[file_index], key, -1);

                Message responseMessage;
                responseMessage.isServer = true;
                responseMessage.messageType = DELETE_REQUEST;
                responseMessage.success = true;
                responseMessage.valueSize = 0;
                responseMessage.key = key;
                strncpy(responseMessage.value, "", vsize);
                responseMessage.id = id;

                if (mq_send(mq2, (const char *)&responseMessage, sizeof(Message), 0) == -1)
                {
                    printf("Error sending successful DELETE_RESPONSE message to mq2");
                    exit(EXIT_FAILURE);
                }

                printf("Thread %d: Key %ld deleted\n", thread_id, key);
            }
            else
            {
                Message responseMessage;
                responseMessage.isServer = true;
                responseMessage.messageType = DELETE_REQUEST;
                responseMessage.success = false;
                responseMessage.valueSize = 0;
                responseMessage.key = key;
                strncpy(responseMessage.value, "", vsize);
                responseMessage.id = id;

                if (mq_send(mq2, (const char *)&responseMessage, sizeof(Message), 0) == -1)
                {
                    printf("Error sending failed DELETE_RESPONSE message to mq2, key does not exist");
                    exit(EXIT_FAILURE);
                }

                printf("Thread %d: Key %ld not found\n", thread_id, key);
            }
            break;
        }

        case GET_REQUEST:
        {
            size_t offset = getOffsetForKey(key, &globalHashTables[file_index]);

            if (offset != -1)
            {
                FILE *file = filePointers[file_index];
                fseek(file, offset, SEEK_SET);

                char *getValue = (char *)malloc(vsize);
                fread(getValue, sizeof(char), vsize, file);

                Message responseMessage;
                responseMessage.isServer = true;
                responseMessage.messageType = GET_REQUEST;
                responseMessage.success = true;
                responseMessage.valueSize = vsize;
                responseMessage.key = key;
                strncpy(responseMessage.value, getValue, vsize);
                responseMessage.id = id;

                if (mq_send(mq2, (const char *)&responseMessage, sizeof(Message), 0) == -1)
                {
                    printf("Error sending successful GET_RESPONSE message to mq2");
                    exit(EXIT_FAILURE);
                }

                printf("Thread %d: Key %ld, Value: %s\n", thread_id, key, value);
                free(value);
            }
            else
            {
                Message responseMessage;
                responseMessage.isServer = true;
                responseMessage.messageType = GET_REQUEST;
                responseMessage.success = false;
                responseMessage.valueSize = 0;
                responseMessage.key = key;
                strncpy(responseMessage.value, "", vsize);
                responseMessage.id = id;

                if (mq_send(mq2, (const char *)&responseMessage, sizeof(Message), 0) == -1)
                {
                    printf("Error sending failed GET_RESPONSE message to mq2, key does not exist");
                    exit(EXIT_FAILURE);
                }

                printf("Thread %d: Key %ld not found\n", thread_id, key);
            }

            break;
        }
        case DUMP:
        {
            FILE *outputFile = fopen(value, "w");

            if (outputFile == NULL)
            {
                printf("Error opening output file");
                exit(EXIT_FAILURE);
            }

            for (int i = 0; i < dcount; ++i)
            {
                FILE *binaryFile = filePointers[i];
                fseek(binaryFile, 0, SEEK_SET);

                if (binaryFile == NULL)
                {
                    fprintf(stderr, "Error: File pointer at index %d is NULL\n", i);
                    continue;
                }

                long int k;
                char *val = (char *)malloc(vsize);
                printf("File %d:\n", i + 1);
                while (fread(&k, sizeof(long int), 1, binaryFile) == 1)
                {
                    if (k < 1 || k == 4294967295)
                    {
                        // Treat the key-value pair as deleted, seek to the next key offset
                        fseek(binaryFile, vsize, SEEK_CUR);
                        continue;
                    }

                    fread(val, vsize, 1, binaryFile);
                    fprintf(outputFile, "Key: %ld, Value: %s\n", k, val);
                }

                free(val);
            }

            fclose(outputFile);

            Message responseMessage;
            responseMessage.isServer = true;
            responseMessage.messageType = DUMP;
            responseMessage.success = true;
            responseMessage.valueSize = 0;
            responseMessage.key = -1;
            ;
            strncpy(responseMessage.value, "", vsize);
            responseMessage.id = id;

            if (mq_send(mq2, (const char *)&responseMessage, sizeof(Message), 0) == -1)
            {
                printf("Error sending failed DUMP_RESPONSE message to mq2");
                exit(EXIT_FAILURE);
            }
            break;
        }
        case QUITSERVER:
        {
            pthread_mutex_lock(&quit_mutex);
            pthread_cond_signal(&quit_cond);
            should_exit = true;
            pthread_mutex_unlock(&quit_mutex);
            pthread_cond_signal(&mq1_empty);
            pthread_exit(NULL);
        }

        default:
            printf("Error: Invalid message type");
            break;
        }

        pthread_mutex_unlock(&file_mutex[file_index]);

        printf("Worker Thread %d is processing Message ID: %d\n", thread_id, messageType);

        pthread_cond_signal(&mq1_empty);
    }

    pthread_exit(NULL);
}

void *frontend(void *arg)
{
    printf("Server frontend running \n");

    while (!should_exit)
    {
        Message newMessage;
        if (mq_receive(mq1, (char *)&newMessage, sizeof(Message), NULL) > 0)
        {
            pthread_mutex_lock(&buffer_mutex);

            printf("Server received message\n");
            while (buffer_size == BUFFER_SIZE)
            {
                printf("Server message buffer is full\n");
                pthread_cond_wait(&mq1_empty, &buffer_mutex);
            }

            buffer[rear] = newMessage;
            rear = (rear + 1) % BUFFER_SIZE;
            buffer_size++;

            pthread_cond_signal(&mq1_full);
            pthread_mutex_unlock(&buffer_mutex);
        }

        if (should_exit)
        {
            pthread_exit(NULL);
        }
    }

    pthread_cond_broadcast(&mq1_full);
    pthread_mutex_unlock(&buffer_mutex);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    if (argc != 11)
    {
        printf("Usage: ./serverk -d <dcount> -f <fname> -t <tcount> -s <vsize> -m <mqname>\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; i += 2)
    {
        if (i + 1 >= argc)
        {
            printf("Invalid number of arguments\n");
            exit(EXIT_FAILURE);
        }

        if (strcmp(argv[i], "-d") == 0)
        {
            dcount = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-f") == 0)
        {
            strncpy(fname, argv[i + 1], sizeof(fname));
        }
        else if (strcmp(argv[i], "-t") == 0)
        {
            tcount = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-s") == 0)
        {
            vsize = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], "-m") == 0)
        {
            strncpy(mqname, argv[i + 1], sizeof(mqname));
        }
        else
        {
            printf("Invalid arguments\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("dcount: %d\n", dcount);
    printf("fname: %s\n", fname);
    printf("tcount: %d\n", tcount);
    printf("vsize: %d\n", vsize);
    printf("mqname: %s\n", mqname);

    char mqname1[256], mqname2[256];

    snprintf(mqname1, sizeof(mqname1), "/%s1", mqname);
    snprintf(mqname2, sizeof(mqname2), "/%s2", mqname);

    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = sizeof(Message);
    attr.mq_curmsgs = 0;
    mq1 = mq_open(mqname1, O_CREAT | O_RDWR, 0666, &attr);
    mq2 = mq_open(mqname2, O_CREAT | O_RDWR, 0666, &attr);

    if (mq1 == (mqd_t)-1 || mq2 == (mqd_t)-1)
    {
        printf("Error opening message queues");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("Q opened %s and %s\n", mqname1, mqname2);
    }

    for (int i = 1; i <= dcount; ++i)
    {
        char filename[256];
        snprintf(filename, sizeof(filename), "%s%d%s", fname, i, ".bin");

        filePointers[i - 1] = fopen(filename, "ab+");
        if (filePointers[i - 1] == NULL)
        {
            fprintf(stderr, "Failed to open file: %s\n", filename);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < dcount; ++i)
    {
        FILE *file = filePointers[i];

        initHashTable(&globalHashTables[i], 1024);

        long int key;
        size_t offset = 0;

        while (fread(&key, sizeof(long int), 1, file) == 1)
        {

            if (key != deletion_marker)
            {
                updateHashTable(&globalHashTables[i], key, offset + sizeof(long int));
            }

            fseek(file, vsize, SEEK_CUR);
            offset += sizeof(long int) + vsize;
        }
    }

    pthread_t worker_threads[MAX_FILES];
    pthread_t frontend_thread;
    int thread_ids[tcount];

    for (int i = 0; i < tcount; ++i)
    {
        thread_ids[i] = i + 1;
        if (pthread_create(&worker_threads[i], NULL, worker, (void *)&thread_ids[i]) != 0)
        {
            printf("Error creating worker thread");
            exit(EXIT_FAILURE);
        }
    }

    if (pthread_create(&frontend_thread, NULL, frontend, NULL) != 0)
    {
        printf("Error creating frontend thread");
        exit(EXIT_FAILURE);
    }

    printf("WAITING FOR QUIT SIGNAL");
    pthread_mutex_lock(&quit_mutex);
    pthread_cond_wait(&quit_cond, &quit_mutex);
    printf("QUIT SIGNAL RECEVIED");

    for (int i = 0; i < dcount; ++i)
    {
        fclose(filePointers[i]);
    }

    pthread_cond_destroy(&mq1_empty);
    pthread_cond_destroy(&mq1_full);

    mq_close(mq1);
    mq_close(mq2);

    printf("SERVER EXIT");

    return 0;
}