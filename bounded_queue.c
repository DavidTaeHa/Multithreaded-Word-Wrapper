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

struct queue_data
{
    char *filename;
    struct queue *temp_queue;
};

// initializes the queue
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

// Adds name to the queue
void *enqueue(void *arguments)
{
    struct queue_data *data = (struct queue_data *)arguments;
    char *n = data->filename;
    struct queue *q = data->temp_queue;
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
}

// Dequeues names from the queue
void *dequeue(void *arguments)
{
    struct queue_data *data = (struct queue_data *)arguments;
    char **n = &data->filename;
    struct queue *q = data->temp_queue;
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
    pthread_cond_signal(&q->enqueue_ready);
    pthread_mutex_unlock(&q->lock);
}

// Prints out all elements within the queue for testing
void print_queue(struct queue *q)
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

int main()
{
    pthread_t tid;
    pthread_t tid2;
    struct queue *temp = malloc(sizeof(struct queue));
    queue_init(temp);
    struct queue_data *enqueue_temp = malloc(sizeof(struct queue_data));
    enqueue_temp->filename = "I believe I can fly";
    enqueue_temp->temp_queue = temp;
    pthread_create(&tid, NULL, &enqueue, (void *)enqueue_temp);
    /*
    char *phrase;
    struct queue_data *dequeue_temp = malloc(sizeof(struct queue_data));
    dequeue_temp->filename = phrase;
    dequeue_temp->temp_queue = temp;
    pthread_create(&tid2, NULL, &dequeue, (void *)dequeue_temp);
    pthread_join(tid, NULL);
    */
    pthread_join(tid, NULL);
    // dequeue_old(&phrase, temp);
    //printf("%s\n", dequeue_temp->filename);
}