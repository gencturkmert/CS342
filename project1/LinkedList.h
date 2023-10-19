#ifndef LINKED_LIST_H
#define LINKED_LIST_H

struct Node
{
    int data;
    struct Node *next;
};

struct LinkedList
{
    struct Node *head;
    int size;
};

void initializeLinkedList(struct LinkedList *list);

void addNode(struct LinkedList *list, int data);

struct Node *createNode(int data);

void freeList(struct LinkedList *list);


int getInt(struct LinkedList *list, int index);


#endif