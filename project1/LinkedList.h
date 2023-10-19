#ifndef LINKED_LIST_H
#define LINKED_LIST_H

struct Node
{
    void *data;
    struct Node *next;
};

struct LinkedList
{
    struct Node *head;
    size_t size;
};

void initializeLinkedList(struct LinkedList *list);

void addNode(struct LinkedList *list, void *data);

void freeList(struct LinkedList *list);

void addIntNode(struct LinkedList *list, int value);

void addStringNode(struct LinkedList *list, const char *value);

int getInt(struct LinkedList *list, int index);

const char *getString(struct LinkedList *list, int index);

#endif