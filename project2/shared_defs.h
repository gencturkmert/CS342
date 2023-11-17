#ifndef SHARED_DEFS_H
#define SHARED_DEFS_H

#include <stdbool.h>
#include <pthread.h>

#define MAX_KEY_SIZE 1024

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
    long int key;
    char *value;
    int id;
} Message;

#endif // SHARED_DEFS_H