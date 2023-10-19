#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <time.h>
#include <mqueue.h>
#include "LinkedList.h"

#define MAX_INT_PER_MESSAGE 21
#define MAX_PRIME_ARRAY_SIZE 21
#define MESSAGE_SIZE sizeof(struct msg_buffer)
#define MESSAGE_QUEUE_NAME "/message_queue"

struct msg_buffer
{
    long msg_type;
    int data[MAX_INT_PER_MESSAGE];
    int is_termination; // Added to indicate termination
};

mqd_t mq;

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

void divideInput(const char *inputfile, int N, struct LinkedList *fileList)
{
    FILE *input = fopen(inputfile, "r");
    if (input == NULL)
    {
        perror("Error opening input file");
        exit(EXIT_FAILURE);
    }

    struct LinkedList *valueList;

    int num;
    while (fscanf(input, "%d", &num) == 1)
    {
        addIntNode(&valueList, num);
    }

    fclose(input);

    int portion_size = valueList->size / N;

    struct Node *current = valueList->head;
    for (int i = 0; i < N; ++i)
    {
        char *filename == NULL;
        if (asprintf(&filename, "tempfile%d.txt", i) == -1)
        {
            perror("Memory allocation failed for  file: %d \n", i);
            exit(EXIT_FAILURE);
        }

        addStringNode(&fileList, filename);

        FILE *intermediateFile = fopen(filename, "w");
        if (intermediateFile == NULL)
        {
            perror("Error opening intermediate file %d", i);
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < portion_size; ++j)
        {
            if (current != NULL)
            {
                fprintf(intermediateFile, "%d\n", num);
                current = current->next;
            }
        }

        if (i == N - 1)
        {
            while (current != NULL)
            {
                num = *((int *)(current->data))
                          fprintf(intermediateFile, "%d\n", num);
                current = current->next;
            }
        }

        fclose(intermediateFile);
    }

    freeList(valueList);
}

void processFile(const char *filename, mqd_t mq, int M)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    struct msg_buffer msg;
    int num;
    int prime_array[MAX_PRIME_ARRAY_SIZE];
    int prime_count = 0;

    while (fscanf(file, "%d", &num) == 1)
    {
        if (isPrime(num))
        {
            prime_array[prime_count++] = num;

            if (prime_count == M)
            {
                msg.msg_type = getpid();
                for (int i = 0; i < M; ++i)
                {
                    msg.data[i] = prime_array[i];
                }

                msg.is_termination = 0; // Normal message, not termination
                if (mq_send(mq, (const char *)&msg, MESSAGE_SIZE, 0) == -1)
                {
                    perror("Error sending message");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    printf("Message sent from child\n");
                }

                prime_count = 0;

                for (int i = 0; i < M; i++)
                {
                    prime_array[i] = 0;
                }
            }
        }
    }

    if (prime_count > 0)
    {
        msg.msg_type = getpid();
        for (int i = 0; i < prime_count; ++i)
        {
            msg.data[i] = prime_array[i];
        }

        msg.is_termination = 0; // Normal message, not termination
        if (mq_send(mq, (const char *)&msg, MESSAGE_SIZE, 0) == -1)
        {
            perror("Error sending message");
            exit(EXIT_FAILURE);
        }
    }

    fclose(file);

    // Send termination message after closing the file
    msg.msg_type = getpid();
    msg.is_termination = 1; // Termination message
    if (mq_send(mq, (const char *)&msg, MESSAGE_SIZE, 0) == -1)
    {
        perror("Error sending termination message");
        exit(EXIT_FAILURE);
    }

    if (remove(filename) != 0)
    {
        perror("Error deleting intermediate file %s \n", filename);
    }
}

int main(int argc, char *argv[])
{
    if (argc != 6)
    {
        printf("Usage: %s N M inputfile outputfile\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    clock_t start_time = clock();

    int N = atoi(argv[1]);
    int M = atoi(argv[2]);
    const char *inputfile = argv[3];
    const char *outputfile = argv[4];

    // Divide input file to intermediate files
    struct LinkedList interFileList;
    initializeLinkedList(&interFileList); // Corrected the function call
    divideInput(inputfile, N, &interFileList);

    // Create message queue
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MESSAGE_SIZE;
    attr.mq_curmsgs = 0;

    mq = mq_open(MESSAGE_QUEUE_NAME, O_CREAT | O_RDONLY, 0666, &attr);
    if (mq == (mqd_t)-1)
    {
        perror("Error creating message queue");
        exit(EXIT_FAILURE);
    }

    // Opening output file
    FILE *output = fopen(outputfile, "w");
    if (output == NULL)
    {
        perror("Error opening output file");
        exit(EXIT_FAILURE);
    }

    // Create child processes for each intermediate file
    for (int i = 0; i < N; ++i)
    {
        pid_t pid = fork();

        if (pid == -1)
        {
            perror("Error forking");
            exit(EXIT_FAILURE);
        }

        if (pid == 0)
        {
            // Child process
            processFile(getString(&interFileList, i), mq, M);
            exit(EXIT_SUCCESS);
        }
    }

    struct msg_buffer msg;
    while (counter < N)
    {
        mq_receive(mq, (char *)&msg, MESSAGE_SIZE, NULL);

        if (msg.is_termination)
        {
            counter++;
        }
        else
        {
            printf("Received message from child: ");
            for (int i = 0; i < MAX_MESSAGES; ++i)
            {
                fprintf(output, "%d/n ", msg.data[i]);
            }
            printf("\n");
        }
    }

    // Parent terminates after receiving N termination messages
    printf("Received %d termination messages. Parent is terminating.\n", N);

    fclose(output);

    freeList(&interFileList);

    if (mq_close(mq) == -1)
    {
        perror("Error closing message queue");
        exit(EXIT_FAILURE);
    }

    mq_unlink(MESSAGE_QUEUE_NAME);

    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Total time taken: %.2f seconds\n", elapsed_time);

    return 0;
}
