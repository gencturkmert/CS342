#include "clocklist.h"
#include <stdio.h>
#include <stdlib.h>

void initializeList(struct ClockList *list)
{
    list->head = NULL;
}

void insertAtBeginning(struct ClockList *list, unsigned int data)
{
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    newNode->data = data;

    if (list->head == NULL)
    {
        newNode->next = newNode;
        list->head = newNode;
    }
    else
    {
        newNode->next = list->head->next;
        list->head->next = newNode;
    }
}

void removeFromList(struct ClockList *list, unsigned int data)
{
    if (list->head == NULL)
    {
        return;
    }

    struct Node *current = list->head;
    struct Node *prev = NULL;

    do
    {
        if (current->data == data)
        {
            if (prev == NULL)
            {
                if (current->next == current)
                {
                    list->head = NULL;
                }
                else
                {
                    list->head = current->next;
                }
            }
            else
            {
                prev->next = current->next;
            }
            free(current);
            return;
        }

        prev = current;
        current = current->next;

    } while (current != list->head);
}

void addToTail(struct ClockList *list, unsigned int data)
{
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    newNode->data = data;

    if (list->head == NULL)
    {
        newNode->next = newNode;
        list->head = newNode;
    }
    else
    {
        newNode->next = list->head->next;
        list->head->next = newNode;
        list->head = newNode;
    }
}

void printList(struct ClockList *list)
{
    if (list->head == NULL)
    {
        printf("List is empty\n");
        return;
    }

    struct Node *current = list->head;
    do
    {
        printf("%u ", current->data);
        current = current->next;
    } while (current != list->head);
    printf("\n");
}
