#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include "file_queue.h"

int MAXSIZE = 16;

// Unbounded_queue for the directory paths

// initializes the queue
int file_init(struct file_queue *q, int count)
{
    q->start = 0;
    q->stop = 0;
    q->isEmpty = 1;
    q->names = malloc(sizeof(char *) * MAXSIZE);
    for (int i = 0; i < MAXSIZE; i++)
    {
        q->names[i] = NULL;
    }
    pthread_mutex_init(&q->lock, NULL);
    pthread_mutex_init(&q->lock2, NULL);
    pthread_cond_init(&q->dequeue_ready, NULL);
    return 0;
}

// Frees the queue
int file_destroy(struct file_queue *q)
{
    for (int i = 0; i < MAXSIZE; i++)
    {
        if (q->names[i] != NULL)
        {
            free(q->names[i]);
        }
    }
    free(q->names);
    pthread_mutex_destroy(&q->lock);
    pthread_mutex_destroy(&q->lock2);
    pthread_cond_destroy(&q->dequeue_ready);
}

// Adds name to the queue
int file_enqueue(char *n, struct file_queue *q)
{
    pthread_mutex_lock(&q->lock);
    int return_result = 1;
    if (DEBUG)
        printf("Enqueueing \'%s\'...\n", n);

    // Increases size of queue
    if (q->stop == MAXSIZE)
    {
        if (DEBUG)
            printf("Increasing size...\n");

        // Doubles size of queue
        q->names = realloc(q->names, 2 * MAXSIZE * sizeof(char *));
        for (int i = MAXSIZE; i < (MAXSIZE * 2); i++)
        {
            q->names[i] = NULL;
        }

        // Add the new bigger queue to the struct
        MAXSIZE *= 2;
    }

    // Adds item to the queue and increments end of queue
    q->names[q->stop] = n;
    q->stop++;
    q->isEmpty = 0;
    return_result = 0;
    pthread_cond_signal(&q->dequeue_ready);
    pthread_mutex_unlock(&q->lock);
    return return_result;
}

// Dequeues names from the queue
int file_dequeue(char **n, struct file_queue *q)
{
    pthread_mutex_lock(&q->lock);

    // Currently queue is empty and must wait
    while (q->isEmpty == 1)
    {
        printf("STUCK WAITING\n");
        pthread_cond_wait(&q->dequeue_ready, &q->lock);
    }

    // Dequeues item and increments start of queue
    if (DEBUG)
        printf("Dequeueing \'%s\'...\n", *n);
    *n = q->names[q->start];
    q->start++;

    // Checks if queue is empty
    if (q->stop == q->start)
    {
        q->isEmpty = 1;
    }
    pthread_mutex_unlock(&q->lock);
    return 0;
}

// Prints out all elements within the queue for testing
void file_print(struct file_queue *q)
{
    printf("Printing list...\n");
    for (int i = 0; i < MAXSIZE; i++)
    {
        if (q->names[i] == NULL)
        {
            printf("(EMPTY)\n");
        }
        else
        {
            printf("%s\n", q->names[i]);
        }
    }
}