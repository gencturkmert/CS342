#ifndef SHARED_DEFS_H
#define SHARED_DEFS_H

#include <stdbool.h>

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

#endif // SHARED_DEFS_H