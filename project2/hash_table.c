#include "hash_table.h"

// Function to initialize a hash table
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

// Function to get the offset from the hash table based on the given key
size_t getOffsetForKey(const long int key, const HashTable *hashTable)
{
    size_t hash = key % hashTable->table_size;

    // Acquire the lock for the hash table
    pthread_mutex_lock(&(hashTable->table_mutex));

    // Find the entry in the hash table
    size_t i = hash;
    while (hashTable->table[i].key != -1 && hashTable->table[i].key != key)
    {
        i = (i + 1) % hashTable->table_size;
    }

    // Get the offset from the hash table (if the key is found)
    size_t offset = (hashTable->table[i].key == key) ? hashTable->table[i].offset : -1;

    // Release the lock for the hash table
    pthread_mutex_unlock(&(hashTable->table_mutex));

    return offset;
}
