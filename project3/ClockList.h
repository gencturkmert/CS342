#ifndef CLOCKLIST_H
#define CLOCKLIST_H

struct Node
{
    unsigned int data;
    struct Node *next;
};

struct ClockList
{
    struct Node *head;
    struct Node* tail;
};

void initializeList(struct ClockList *list);
void removeFromList(struct ClockList *list, unsigned int data);
void addToTail(struct ClockList *list, unsigned int data);
void printList(struct ClockList *list);

#endif // CLOCKLIST_H
