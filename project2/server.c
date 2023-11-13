#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mqueue.h>
#include <pthread.h>
#include "shared_defs.h"

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

// Global HashTable Array
HashTable globalHashTables[MAX_FILES];

void *worker(void *arg)
{
    int thread_id = *((int *)arg);

    while (1)
    {
        // Access files in a synchronized manner
        pthread_mutex_lock(&worker_mutex);

        // Access the buffer in a synchronized manner
        pthread_mutex_lock(&buffer_mutex);

        // Wait until the buffer is not empty
        while (buffer_size == 0)
        {
            pthread_cond_wait(&mq1_full, &buffer_mutex);
        }

        // Perform operations on the buffer or any other shared resources
        Message message = buffer[front];
        front = (front + 1) % BUFFER_SIZE;
        buffer_size--;

        // Release the buffer mutex
        pthread_mutex_unlock(&buffer_mutex);

        int file_index = (message.key % ddcount) + 1;

        pthread_mutex_lock(&file_mutex[file_index]);

        // Process the message and access the file
        // ...

        // Release the file lock
        pthread_mutex_unlock(&file_mutex[file_index]);

        printf("Worker Thread %d is processing Message ID: %d, Data: %s\n", thread_id, message.messageType, message.value);

        // Signal that the buffer is not full
        pthread_cond_signal(&mq1_empty);
    }
}

// Frontend Function
void *frontend(void *arg)
{
    // Access message queue in a synchronized manner
    pthread_mutex_lock(&buffer_mutex);

    while (1)
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

void initHashTable(HashTable *hashTable, size_t size)
{
    hashTable->table_size = size;
    hashTable->table = (DataItem *)malloc(size * sizeof(DataItem));
    if (hashTable->table == NULL)
    {
        perror("Error initializing hash table");
        exit(EXIT_FAILURE);
    }

    // Initialize the table with default values
    for (size_t i = 0; i < size; ++i)
    {
        hashTable->table[i].key = -1; // Assuming -1 indicates an empty slot
        hashTable->table[i].offset = -1;
    }

    // Initialize the mutex for this hash table
    if (pthread_mutex_init(&(hashTable->table_mutex), NULL) != 0)
    {
        perror("Error initializing hash table mutex");
        exit(EXIT_FAILURE);
    }
}
// Function to update the hash table after adding a new data item
void updateHashTable(HashTable *hashTable, long int key, size_t fileOffset)
{
    size_t hash = key % hashTable->table_size;

    // Acquire the lock for the hash table
    pthread_mutex_lock(&(hashTable->table_mutex));

    // Find an empty slot or update existing entry
    size_t i = hash;
    while (hashTable->table[i].key != -1 && hashTable->table[i].key != key)
    {
        i = (i + 1) % hashTable->table_size;
    }

    // Update the hash table with the new data item
    hashTable->table[i].key = key;
    hashTable->table[i].offset = fileOffset;

    // Release the lock for the hash table
    pthread_mutex_unlock(&(hashTable->table_mutex));
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

    mqd_t mq1, mq2;
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

    if (mq1 == (mqd_t)-1 || mq2 == (mqd_t)-1)
    {
        perror("Error opening message queues");
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i <= ddcount; ++i)
    {
        char filename[256];
        snprintf(filename, sizeof(filename), "%s%d", ffname, i);

        filePointers[i - 1] = fopen(filename, "ab+");
        if (filePointers[i - 1] == NULL)
        {
            perror("Error opening or creating data file");
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
            // Update the hash table with the key and its offset
            updateHashTable(&globalHashTables[i], key, offset);

            // Move to the next data item
            fseek(file, svsize, SEEK_CUR);
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
            perror("Error creating worker thread");
            exit(EXIT_FAILURE);
        }
    }

    // Create frontend thread
    if (pthread_create(&frontend_thread, NULL, frontend, NULL) != 0)
    {
        perror("Error creating frontend thread");
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