#include "ClockList.h"
#include <stdio.h>
#include <stdlib.h>
void initializeList(struct ClockList* list) {
    list->head = NULL;
    list->tail = NULL;
}

// Function to remove a node with the specified data from the list
void removeFromList(struct ClockList* list, unsigned int data) {
    struct Node* current = list->head;
    struct Node* previous = NULL;

    while (current != NULL) {
        if (current->data == data) {
            if (previous == NULL) {
                // If the node to be removed is the head
                list->head = current->next;
                free(current);
                return;
            } else {
                previous->next = current->next;
                // If the node to be removed is the tail, update tail
                if (current == list->tail) {
                    list->tail = previous;
                }
                free(current);
                return;
            }
        }
        previous = current;
        current = current->next;
    }
}

// Function to add a new node with the specified data to the tail of the list
void addToTail(struct ClockList* list, unsigned int data) {
    struct Node* current = list->head;
    while (current != NULL) {
        if (current->data == data) {
            return;
        }
        current = current->next;
    }
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    if (newNode == NULL) {
        fprintf(stderr, "Memory allocation error\n");
        exit(EXIT_FAILURE);
    }

    newNode->data = data;
    newNode->next = NULL;

    if (list->tail == NULL) {
        // If the list is empty, set both head and tail to the new node
        list->head = newNode;
        list->tail = newNode;
    } else {
        // Add the new node to the tail and update the tail
        list->tail->next = newNode;
        list->tail = newNode;
    }
}

// Function to print the elements of the list
void printList(struct ClockList* list) {
    struct Node* current = list->head;

    printf("List: ");
    while (current != NULL) {
        printf("%u ", current->data);
        current = current->next;
    }
    printf("\n");
}

// Function to free the memory allocated for the nodes in the list
void freeList(struct ClockList* list) {
    struct Node* current = list->head;
    struct Node* next;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    // Reset the list to an empty state
    list->head = NULL;
    list->tail = NULL;
}