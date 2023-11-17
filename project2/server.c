#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <pthread.h>
#include "shared_defs.h"
#include "hash_table.h"

#define BUFFER_SIZE 128
#define MAX_FILES 5

int serverk;
int ddcount;
char ffname[256];
int ttcount;
int svsize;
char mmqname[256];

// Global File Pointers Array
FILE *filePointers[MAX_FILES];
pthread_mutex_t file_mutex[MAX_FILES];
pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mq1_empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t mq1_full = PTHREAD_COND_INITIALIZER;

// Circular Buffer
Message buffer[BUFFER_SIZE];

int front = 0;       // Front index of the circular buffer
int rear = 0;        // Rear index of the circular buffer
int buffer_size = 0; // Number of messages in the buffer

int deletion_marker = -1;
bool should_exit = false;

// Global HashTable Array
HashTable globalHashTables[MAX_FILES];

mqd_t mq1, mq2;

void *worker(void *arg)
{
    int thread_id = *((int *)arg);

    while (!should_exit)
    {
        pthread_mutex_lock(&buffer_mutex);

        while (buffer_size == 0)
        {
            pthread_cond_wait(&mq1_full, &buffer_mutex);
        }

        MessageType messageType = buffer[front].messageType;
        long int key = buffer[front].key;
        char *value = buffer[front].value;
        bool quit = buffer[front].quit;

        if (quit)
        {
            should_exit = true;
        }

        int file_index = (message.key % ddcount);

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
                if (strlen(value) < svsize)
                {
                    fwrite(value, sizeof(char), strlen(value), file);
                    size_t remainingBytes = svsize - strlen(value);
                    char nullBytes[remainingBytes];
                    memset(nullBytes, 0, remainingBytes);
                    fwrite(nullBytes, sizeof(char), remainingBytes, file);
                }
                else
                {
                    fwrite(value, sizeof(char), svsize, file);
                }
            }
            else
            {
                fseek(file, 0, SEEK_END);
                fwrite(&key, sizeof(long int), 1, file);
                if (strlen(value) < svsize)
                {
                    fwrite(value, sizeof(char), strlen(value), file);
                    size_t remainingBytes = svsize - strlen(value);
                    char nullBytes[remainingBytes];
                    memset(nullBytes, 0, remainingBytes);
                    fwrite(nullBytes, sizeof(char), remainingBytes, file);
                }
                else
                {
                    fwrite(value, sizeof(char), svsize, file);
                }

                size_t newOffset = ftell(file) - svsize;
                updateHashTable(&globalHashTables[file_index], key, newOffset);
            }

            Message responseMessage;
            responseMessage.isServer = true;
            responseMessage.quit = false;
            responseMessage.messageType = PUT_REQUEST;
            responseMessage.success = true;
            responseMessage.keySize = sizeof(long int);
            responseMessage.valueSize = 0;
            responseMessage.key = key;
            responseMessage.value = NULL;

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
                fwrite(&deletionMarker, sizeof(long int), 1, file);

                Message responseMessage;
                responseMessage.isServer = true;
                responseMessage.quit = false;
                responseMessage.messageType = DELETE_REQUEST;
                responseMessage.success = true;
                responseMessage.keySize = sizeof(long int);
                responseMessage.valueSize = 0;
                responseMessage.key = key;
                responseMessage.value = NULL;

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
                responseMessage.quit = false;
                responseMessage.messageType = DELETE_REQUEST;
                responseMessage.success = false;
                responseMessage.keySize = sizeof(long int);
                responseMessage.valueSize = 0;
                responseMessage.key = key;
                responseMessage.value = NULL;

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

                char *value = (char *)malloc(svsize);
                fread(value, sizeof(char), svsize, file);

                Message responseMessage;
                responseMessage.isServer = true;
                responseMessage.quit = false;
                responseMessage.messageType = GET_REQUEST;
                responseMessage.success = true;
                responseMessage.keySize = sizeof(long int);
                responseMessage.valueSize = svsize;
                responseMessage.key = key;
                responseMessage.value = value;

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
                responseMessage.quit = false;
                responseMessage.messageType = GET_REQUEST;
                responseMessage.success = false;
                responseMessage.keySize = sizeof(long int);
                responseMessage.valueSize = 0;
                responseMessage.key = key;
                responseMessage.value = NULL;

                if (mq_send(mq2, (const char *)&responseMessage, sizeof(Message), 0) == -1)
                {
                    printf("Error sending failed GET_RESPONSE message to mq2, key does not exist");
                    exit(EXIT_FAILURE);
                }

                printf("Thread %d: Key %ld not found\n", thread_id, key);
            }

            break;
        }

        default:
            printf("Error: Invalid message type");
            break;
        }

        pthread_mutex_unlock(&file_mutex[file_index]);

        printf("Worker Thread %d is processing Message ID: %d\n", thread_id, message.messageType);
        pthread_cond_signal(&mq1_empty);
    }
}

// Frontend Function
void *frontend(void *arg)
{
    // Access message queue in a synchronized manner
    pthread_mutex_lock(&buffer_mutex);

    while (should_exit)
    {
        // Check if there is a message in mq1
        Message newMessage;
        if (mq_receive(mq1, (char *)&newMessage, sizeof(Message), NULL) > 0)
        {
            // Wait for the buffer to have space before adding a new message
            while (buffer_size == BUFFER_SIZE)
            {
                pthread_mutex_unlock(&buffer_mutex);
                pthread_cond_wait(&mq1_empty, &buffer_mutex);
                pthread_mutex_lock(&buffer_mutex);
            }

            // Add the new message to the buffer
            buffer[rear] = newMessage;
            rear = (rear + 1) % BUFFER_SIZE;
            buffer_size++;

            // Signal that the buffer is not empty
            pthread_cond_signal(&mq1_full);
        }
    }

    // Release the buffer mutex
    pthread_mutex_unlock(&buffer_mutex);

    pthread_exit(NULL);
}

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

    char mqname1[256], mqname2[256];

    snprintf(mqname1, sizeof(mqname1), "/%s1", mmqname);
    snprintf(mqname2, sizeof(mqname2), "/%s2", mmqname);

    mq1 = mq_open(mqname1, O_CREAT | O_RDWR, 0666, NULL);
    mq2 = mq_open(mqname2, O_CREAT | O_RDWR, 0666, NULL);

    if (mq1 == (mqd_t)-1 || mq2 == (mqd_t)-1)
    {
        printf("Error opening message queues");
        exit(EXIT_FAILURE);
    }

    if (mq1 == (mqd_t)-1 || mq2 == (mqd_t)-1)
    {
        printf("Error opening message queues");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i <= ddcount; ++i)
    {
        char filename[256];
        snprintf(filename, sizeof(filename), "%s%d", ffname, i);

        filePointers[i - 1] = fopen(filename, "ab+");
        if (filePointers[i - 1] == NULL)
        {
            fprintf(stderr, "Failed to open file: %s\n", filename);
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < ddcount; ++i)
    {
        // Open data file for reading
        FILE *file = filePointers[i];

        // Create a hash table for this data file
        initHashTable(&globalHashTables[i], 1024); // You can adjust the size of the hash table as needed

        // Read the data file and generate the hash table
        long int key;
        size_t offset = 0;

        while (fread(&key, sizeof(long int), 1, file) == 1)
        {

            if (key != deletion_marker)
            {
                updateHashTable(&globalHashTables[i], key, offset + sizeof(long int));
            }

            // Move to the next data item
            fseek(file, svsize, SEEK_CUR); // Adjust the offset accordingly
            offset += sizeof(long int) + svsize;
        }
    }

    // Initialize and create threads
    pthread_t worker_threads[ttcount];
    pthread_t frontend_thread;

    int thread_ids[ttcount];

    // Create worker threads
    for (int i = 0; i < ttcount; ++i)
    {
        thread_ids[i] = i + 1;
        if (pthread_create(&worker_threads[i], NULL, worker, (void *)&thread_ids[i]) != 0)
        {
            printf("Error creating worker thread");
            exit(EXIT_FAILURE);
        }
    }

    // Create frontend thread
    if (pthread_create(&frontend_thread, NULL, frontend, NULL) != 0)
    {
        printf("Error creating frontend thread");
        exit(EXIT_FAILURE);
    }

    // Wait for worker threads to finish (this part will not be reached in the infinite loop scenario)
    for (int i = 0; i < ttcount; ++i)
    {
        pthread_join(worker_threads[i], NULL);
    }

    // Close the file pointers
    for (int i = 0; i < ddcount; ++i)
    {
        fclose(filePointers[i]);
    }
    // Close the message queues
    mq_close(mq1);
    mq_close(mq2);
    return 0;
}