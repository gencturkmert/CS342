#ifndef SHARED_DEFS_H
#define SHARED_DEFS_H

#include <stdbool.h>
#include <pthread.h>

#define MAX_KEY_SIZE 1024
#define MAX_VALUE_SIZE 1024

typedef enum
{
    PUT_REQUEST,
    DELETE_REQUEST,
    GET_REQUEST
} MessageType;

typedef struct
{
    bool isServer;
    bool quit;
    MessageType messageType;
    bool success;
    size_t keySize;
    size_t valueSize;
    char *key;
    char *value;
} Message;

// Struct for data items
typedef struct
{
    long int key;
    size_t offset;
} DataItem;

// Struct for hash table
typedef struct
{
    size_t table_size;
    DataItem *table;             // An array to store file offsets (positions) based on hashed keys
    pthread_mutex_t table_mutex; // Mutex for synchronization
} HashTable;

#endif // SHARED_DEFS_H