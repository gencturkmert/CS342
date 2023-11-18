#include "hash_table.h"

void initHashTable(HashTable *hashTable, size_t size)
{
    hashTable->table_size = size;
    hashTable->table = (DataItem *)malloc(size * sizeof(DataItem));
    if (hashTable->table == NULL)
    {
        printf("Error initializing hash table");
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < size; ++i)
    {
        hashTable->table[i].key = -1;
        hashTable->table[i].offset = -1;
    }


}

void updateHashTable(HashTable *hashTable, long int key, size_t fileOffset)
{
    size_t hash = key % hashTable->table_size;


    size_t i = hash;
    while (hashTable->table[i].key != -1 && hashTable->table[i].key != key)
    {
        i = (i + 1) % hashTable->table_size;
    }

    hashTable->table[i].key = key;
    hashTable->table[i].offset = fileOffset;

}

size_t getOffsetForKey(const long int key, const HashTable *hashTable)
{
    size_t hash = key % hashTable->table_size;


    size_t i = hash;
    while (hashTable->table[i].key != -1 && hashTable->table[i].key != key)
    {
        i = (i + 1) % hashTable->table_size;
    }

    size_t offset = (hashTable->table[i].key == key) ? hashTable->table[i].offset : -1;


    return offset;
}
