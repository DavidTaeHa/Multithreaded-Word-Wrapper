#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include <string.h>
#include "bounded_queue.h"

// int QUEUESIZE = 16;

// Bounded_queue for the filepaths

// initializes the queue
int bound_init(struct bounded_queue *q)
{
    q->start = 0;
    q->stop = 0;
    q->full = 0;
    q->isEmpty = 1;
    q->dir_finished = 0;
    q->names = malloc(sizeof(char *) * MAXSIZE);
    for (int i = 0; i < MAXSIZE; i++)
    {
        q->names[i] = NULL;
    }
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->enqueue_ready, NULL);
    pthread_cond_init(&q->dequeue_ready, NULL);
    return 0;
}

// Frees the queue
int bound_destroy(struct bounded_queue *q)
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
    pthread_cond_destroy(&q->enqueue_ready);
    pthread_cond_destroy(&q->dequeue_ready);
}

// Adds name to the queue
int bound_enqueue(char *n, struct bounded_queue *q)
{
    pthread_mutex_lock(&q->lock);
    while (q->full)
    {
        pthread_cond_wait(&q->enqueue_ready, &q->lock);
    }
    q->names[q->stop] = n;
    q->stop++;
    if (q->stop == MAXSIZE)
        q->stop = 0;
    if (q->start == q->stop)
        q->full = 1;
    q->isEmpty = 0;
    pthread_cond_signal(&q->dequeue_ready);
    pthread_mutex_unlock(&q->lock);
    return 0;
}

// Dequeues names from the queue
int bound_dequeue(char **n, struct bounded_queue *q)
{
    pthread_mutex_lock(&q->lock);

    //bounded queue is empty
    while (!q->full && q->start == q->stop)
    {
        //Directory traversal is finished and bounded queue is empty
        if(q->dir_finished == 1){
            printf("DIRECTORY TRAVERSAL IS FINISHED\n");
            pthread_cond_broadcast(&q->dequeue_ready);
            pthread_mutex_unlock(&q->lock);
            return 0;
        }
        pthread_cond_wait(&q->dequeue_ready, &q->lock);
    }
    *n = q->names[q->start];
    q->start++;

    if(!q->full && q->start == q->stop){
        q->isEmpty = 1;
    }

    pthread_cond_signal(&q->enqueue_ready);
    pthread_mutex_unlock(&q->lock);
    return 0;
}

// Prints out all elements within the queue for testing
void bound_print(struct bounded_queue *q)
{
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
/*
int main()
{
    struct bounded_queue *temp = malloc(sizeof(struct bounded_queue));
    bound_init(temp);

    char *path = "boom";
    char *file = "shaka";
    int plen = strlen(path);
    int flen = strlen(file);
    char *newpath = malloc(plen + flen + 2);
    memcpy(newpath, path, plen);
    newpath[plen] = '/';
    memcpy(newpath + plen + 1, file, flen + 1);
    bound_enqueue(newpath, temp);

    bound_print(temp);
    bound_destroy(temp);
    free(temp);
}
*/
//gcc bounded_queue.c