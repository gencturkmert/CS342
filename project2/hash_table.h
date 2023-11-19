// hash_table.h

#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include <stdlib.h>
#include <pthread.h>

typedef struct
{
    long int key;
    size_t offset;
} DataItem;

typedef struct
{
    size_t table_size;
    DataItem *table;
} HashTable;

void initHashTable(HashTable *hashTable, size_t size);
void updateHashTable(HashTable *hashTable, long int key, size_t fileOffset);
size_t getOffsetForKey(const long int key, const HashTable *hashTable);

#endif // HASH_TABLE_H
