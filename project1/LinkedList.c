#include "LinkedList.h"
#include <stdio.h>
#include <stdlib.h>

void initializeLinkedList(struct LinkedList *list)
{
    list->head = NULL;
    list->size = 0;
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

int getInt(struct LinkedList *list, int index) {
    if (index < 0 || index >= list->size) {
        fprintf(stderr, "Index out of bounds.\n");
        exit(1);
    }

    struct Node *current = list->head;
    for (int i = 0; i < index; i++) {
        current = current->next;
    }

    return current->data;
}

struct Node *createNode(int data) {
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(1);
    }
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

void addNode(struct LinkedList *list, int data) {
    struct Node *newNode = createNode(data);

    if (list->head == NULL) {
        list->head = newNode;
    } else {
        struct Node *current = list->head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newNode;
    }

    list->size++;
}

