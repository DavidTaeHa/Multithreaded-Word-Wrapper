#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include "bounded_queue.h"

//int QUEUESIZE = 16;
int total_waiting = 0;

//Bounded_queue for the filepaths

// initializes the queue
int queue_init(struct bounded_queue *q)
{
    q->start = 0;
    q->stop = 0;
    q->full = 0;
    q->wait_status = 0;
    q->capacity = 0;
    q->names = malloc(sizeof(char *) * QUEUESIZE);
    for (int i = 0; i < QUEUESIZE; i++)
    {
        q->names[i] = malloc(sizeof(char) * FILEPATH);
        q->names[i] = NULL;
    }
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->enqueue_ready, NULL);
    pthread_cond_init(&q->dequeue_ready, NULL);
    return 0;
}

// Adds name to the queue
int enqueue(char *n, struct bounded_queue *q)
{
    pthread_mutex_lock(&q->lock);
    while (q->full)
    {
        pthread_cond_wait(&q->enqueue_ready, &q->lock);
    }
    q->names[q->stop] = n;
    q->stop++;
    q->capacity++;
    if (q->stop == QUEUESIZE)
        q->stop = 0;
    if (q->start == q->stop)
        q->full = 1;
    pthread_cond_signal(&q->dequeue_ready);
    pthread_mutex_unlock(&q->lock);
    return 0;
}

// Dequeues names from the queue
int dequeue(char **n, struct bounded_queue *q)
{
    pthread_mutex_lock(&q->lock);
    while (!q->full && q->start == q->stop)
    {
        if (q->wait_status == 0)
        {
            q->wait_status = 1;
            total_waiting++;
        }
        pthread_cond_wait(&q->dequeue_ready, &q->lock);
    }
    if (q->wait_status == 1)
    {
        q->wait_status = 0;
        total_waiting--;
    }
    *n = q->names[q->start];
    q->start++;
    q->capacity--;
    if (q->start == QUEUESIZE)
        q->start == 0;
    q->full = 0;
    pthread_cond_signal(&q->enqueue_ready);
    pthread_mutex_unlock(&q->lock);
    return 0;
}

// Prints out all elements within the queue for testing
void print_queue(struct bounded_queue *q)
{
    for (int i = 0; i < QUEUESIZE; i++)
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