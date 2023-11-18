// queue.h

#ifndef QUEUE_H
#define QUEUE_H

#include "shared_defs.h"
#include <string.h>

typedef struct ResponseNode
{
    Message responseMessage;
    struct ResponseNode *next;
} ResponseNode;

typedef struct
{
    ResponseNode *front;
    ResponseNode *rear;
} ResponseQueue;

void initQueue(ResponseQueue *queue);
void enqueue(ResponseQueue *queue, Message responseMessage);
Message dequeue(ResponseQueue *queue);

#endif // QUEUE_H
