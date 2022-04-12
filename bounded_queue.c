#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>

#ifndef DEBUG
#define DEBUG 1
#endif

#define FILEPATH 4096

int QUEUESIZE = 16;

struct queue
{
    char **names;
    int start, stop;
    int full;
    pthread_mutex_t lock;
    pthread_cond_t enqueue_ready, dequeue_ready;
};

int queue_init(struct queue *q)
{
    q->start = 0;
    q->stop = 0;
    q->full = 0;
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

int enqueue(int n, struct queue *q)
{
    pthread_mutex_lock(&q->lock);
    while (q->full)
    {
        pthread_cond_wait(&q->enqueue_ready, &q->lock);
    }
    q->names[q->stop] = n;
    q->stop++;
    if (q->stop == QUEUESIZE)
        q->stop = 0;
    if (q->start == q->stop)
        q->full = 1;
    pthread_cond_signal(&q->dequeue_ready);
    pthread_mutex_unlock(&q->lock);
    return 0;
}

int dequeue(int *n, struct queue *q)
{
    pthread_mutex_lock(&q->lock);
    while (!q->full && q->start == q->stop)
    {
        pthread_cond_wait(&q->dequeue_ready, &q->lock);
    }
    *n = q->names[q->start];
    q->start++;
    if (q->start == QUEUESIZE)
        q->start == 0;
    q->full = 0;
    pthread_signal(&q->enqueue_ready);
    pthread_mutex_unlock(&q->lock);
    return 0;
}

int main(){
    struct queue *temp = malloc(sizeof(struct queue));
    queue_init(temp);
}