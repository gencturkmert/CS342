#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include "LinkedList.h"
#include <string.h>

#define MAX_INT_PER_MESSAGE 21
#define MAX_PRIME_ARRAY_SIZE 21

#define MAX_THREADS 20

int N;
int M;
const char *inputfile;
const char *outputfile;
struct LinkedList primeLists[MAX_THREADS];
char fileList[20][50];

int isPrime(int num)
{
    if (num <= 1)
    {
        return 0;
    }
    for (int i = 2; i * i <= num; ++i)
    {
        if (num % i == 0)
        {
            return 0;
        }
    }
    return 1;
}

void *processFile(void *arg)
{
    int thread_id = *((int *)arg);

    const char *thrFile = fileList[thread_id];

    FILE *file = fopen(thrFile, "r");
    if (file == NULL)
    {
        printf("Error opening intermediate file %d\n", thread_id);
        exit(EXIT_FAILURE);
    }

    int num;
    while (fscanf(file, "%d", &num) == 1)
    {
        if (isPrime(num))
        {
            addNode(&primeLists[thread_id], num);
        }
    }

    fclose(file);
    return NULL;
}

int main(int argc, char *argv[])
{

    int opt;
    while ((opt = getopt(argc, argv, "n:i:o:")) != -1)
    {
        switch (opt)
        {
        case 'n':
            N = atoi(optarg);
            if (N < 1 || N > MAX_THREADS)
            {
                printf("N must be between 1 and 20.\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'i':
            inputfile = optarg;
            break;
        case 'o':
            outputfile = optarg;
            break;
        default:
            printf("Usage: %s -n N -i INFILE -o OUTFILE\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    clock_t start_time = clock();

    // Divide input file

    FILE *input = fopen(inputfile, "r");
    if (input == NULL)
    {
        printf("Error opening input file");
        exit(EXIT_FAILURE);
    }

    struct LinkedList valueList;
    initializeLinkedList(&valueList);

    int num;
    while (fscanf(input, "%d", &num) == 1)
    {
        addNode(&valueList, num);
    }

    fclose(input);

    int portion_size = valueList.size / N;

    struct Node *current = valueList.head;
    for (int i = 0; i < N; ++i)
    {
        char filename[50];
        sprintf(filename, "tempfile%d.txt", i);

        strcpy(fileList[i], filename);

        FILE *intermediateFile = fopen(filename, "w");
        if (intermediateFile == NULL)
        {
            printf("Error opening intermediate file %d\n", i);
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < portion_size; ++j)
        {
            if (current != NULL)
            {

                fprintf(intermediateFile, "%d\n", current->data);
                current = current->next;
            }
        }

        if (i == N - 1)
        {
            while (current != NULL)
            {
                num = current->data;
                fprintf(intermediateFile, "%d\n", num);
                current = current->next;
            }
        }

        fclose(intermediateFile);
    }

    freeList(&valueList);

    // input divided

    for (int i = 0; i < N; i++)
    {
        initializeLinkedList(&primeLists[i]);
    }

    pthread_t threads[20];
    int thread_ids[20];
    for (int i = 0; i < N; i++)
    {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, processFile, &thread_ids[i]) != 0)
        {
            printf("Error creating thread");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < N; i++)
    {
        if (pthread_join(threads[i], NULL) != 0)
        {
            printf("Error joining thread");
            exit(EXIT_FAILURE);
        }
    }

    FILE *output = fopen(outputfile, "w");
    if (output == NULL)
    {
        printf("Error opening output file");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < N; i++)
    {
        struct Node *current = primeLists[i].head;
        while (current != NULL)
        {
            fprintf(output, "%d\n", current->data);
            current = current->next;
        }
        freeList(&primeLists[i]);

        remove(fileList[i]);
    }

    fclose(output);

    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Total time taken: %.6f seconds\n", elapsed_time);

    return 0;
}