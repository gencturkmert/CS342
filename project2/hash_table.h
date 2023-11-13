// hash_table.h

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdlib.h>
#include <pthread.h>

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

// Function declarations
void initHashTable(HashTable *hashTable, size_t size);
void updateHashTable(HashTable *hashTable, long int key, size_t fileOffset);
size_t getOffsetForKey(const long int key, const HashTable *hashTable);

#endif // HASH_TABLE_H
