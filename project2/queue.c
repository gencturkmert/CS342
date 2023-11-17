// queue.c

#include "queue.h"
#include <stdlib.h>

void initQueue(ResponseQueue *queue)
{
    queue->front = NULL;
    queue->rear = NULL;
}

void enqueue(ResponseQueue *queue, Message responseMessage)
{
    // Create a new node for the response message
    ResponseNode *newNode = (ResponseNode *)malloc(sizeof(ResponseNode));
    newNode->responseMessage = responseMessage;
    newNode->next = NULL;

    // Enqueue the node
    if (queue->rear == NULL)
    {
        queue->front = newNode;
        queue->rear = newNode;
    }
    else
    {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
}

Message dequeue(ResponseQueue *queue)
{
    if (queue->front == NULL)
    {
        Message emptyMessage;
        memset(&emptyMessage, 0, sizeof(Message));
        return emptyMessage;
    }

    ResponseNode *temp = queue->front;
    queue->front = queue->front->next;
    if (queue->front == NULL)
    {
        queue->rear = NULL;
    }

    Message responseMessage = temp->responseMessage;
    free(temp);

    return responseMessage;
}
