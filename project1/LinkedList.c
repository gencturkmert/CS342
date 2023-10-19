#include "LinkedList.h"
#include <stdio.h>
#include <stdlib.h>

void initializeLinkedList(struct LinkedList *list)
{
    list->head = NULL;
    list->size = 0;
}

void addNode(struct LinkedList *list, void *data)
{
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    if (newNode == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    newNode->data = data;

    newNode->next = list->head;

    list->head = newNode;

    list->size++;
}

void freeList(struct LinkedList *list)
{
    struct Node *current = list->head;
    while (current != NULL)
    {
        struct Node *next = current->next;

        free(current);

        current = next;
    }

    list->head = NULL;
    list->size = 0;
}

void addIntNode(struct LinkedList *list, int value)
{
    int *intValue = (int *)malloc(sizeof(int));
    if (intValue == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    *intValue = value;

    addNode(list, intValue);
}

void addStringNode(struct LinkedList *list, const char *value)
{
    char *stringValue = (char *)malloc(strlen(value) + 1);
    if (stringValue == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    strcpy(stringValue, value);

    addNode(list, stringValue);
}

int getInt(struct LinkedList *list, int index)
{
    if (index < 0 || index >= list->size)
    {
        fprintf(stderr, "Index out of bounds for linked list, getInt method\n");
        exit(EXIT_FAILURE);
    }

    struct Node *current = list->head;
    for (int i = 0; i < index; ++i)
    {
        current = current->next;
    }

    return *((int *)(current->data));
}

const char *getString(struct LinkedList *list, int index)
{
    if (index < 0 || index >= list->size)
    {
        fprintf(stderr, "Index out of bounds for linked list, getString method\n");
        exit(EXIT_FAILURE);
    }

    struct Node *current = list->head;
    for (int i = 0; i < index; ++i)
    {
        current = current->next;
    }

    return (const char *)(current->data);
}
