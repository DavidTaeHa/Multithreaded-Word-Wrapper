#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>
#include "unbounded_queue.h"

int QUEUESIZE = 16;

// Unbounded_queue for the directory paths

// initializes the queue
int unbound_init(struct unbounded_queue *q, int count)
{
    q->start = 0;
    q->stop = 0;
    q->isEmpty = 1;
    q->closed = 0;
    q->total_waiting = 0;
    q->thread_count = count;
    q->names = malloc(sizeof(char *) * QUEUESIZE);
    for (int i = 0; i < QUEUESIZE; i++)
    {
        q->names[i] = NULL;
    }
    pthread_mutex_init(&q->lock, NULL);
    pthread_mutex_init(&q->lock2, NULL);
    pthread_cond_init(&q->dequeue_ready, NULL);
    return 0;
}

// Frees the queue
int unbound_destroy(struct unbounded_queue *q)
{
    for (int i = 0; i < QUEUESIZE; i++)
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
int unbound_enqueue(char *n, struct unbounded_queue *q)
{
    pthread_mutex_lock(&q->lock);
    if (q->closed == 1)
    {
        q->isEmpty = 1;
        pthread_cond_broadcast(&q->dequeue_ready);
        pthread_mutex_unlock(&q->lock);
        return 1;
    }
    int return_result = 1;
    if (DEBUG)
        printf("Enqueueing \'%s\'...\n", n);

    // Increases size of queue
    if (q->stop == QUEUESIZE)
    {
        if (DEBUG)
            printf("Increasing size...\n");

        // Doubles size of queue
        q->names = realloc(q->names, 2 * QUEUESIZE * sizeof(char *));
        for (int i = QUEUESIZE; i < (QUEUESIZE * 2); i++)
        {
            q->names[i] = NULL;
        }

        // Add the new bigger queue to the struct
        QUEUESIZE *= 2;
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
int unbound_dequeue(char **n, struct unbounded_queue *q)
{
    pthread_mutex_lock(&q->lock);

    // Add close operation: NOTE dequeue fails if queue is empty and closed
    /*
    if (q->isEmpty == 1)
    {
        if (DEBUG)
            printf("Queue is empty...\n");
    }
    */
    // printf("Waiting Directory Threads: %d\n", q->total_waiting);
    // printf("Empty status: %d\n", q->isEmpty);

    // Currently queue is empty and must wait
    while (q->isEmpty == 1)
    {
        q->total_waiting++;
        if (q->closed == 1)
        {
            pthread_cond_broadcast(&q->dequeue_ready);
            pthread_mutex_unlock(&q->lock);
            return 0;
        }
        if (q->total_waiting == q->thread_count)
        {
            printf("WAITING EQUALS THREAD COUNT\n");
            q->closed = 1;
            pthread_cond_broadcast(&q->dequeue_ready);
            pthread_mutex_unlock(&q->lock);
            return 0;
        }
        pthread_cond_wait(&q->dequeue_ready, &q->lock);
        q->total_waiting--;
    }
    // gcc -fsanitize=address,undefined ww.c file_queue.c unbounded_queue.c
    //./a.out -r1000,1000 15 foldera
    //  Dequeues item and increments start of queue
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
    return 1;
}

// Prints out all elements within the queue for testing
void unbound_print(struct unbounded_queue *q)
{
    printf("Printing list...\n");
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
/*
int main(){
    struct unbounded_queue *temp = malloc(sizeof(struct unbounded_queue));
    unbound_init(temp);

    char *path = "boom";
    char *file = "shaka";
    int plen = strlen(path);
    int flen = strlen(file);
    char *newpath = malloc(plen + flen + 2);
    memcpy(newpath, path, plen);
    newpath[plen] = '/';
    memcpy(newpath + plen + 1, file, flen + 1);
    unbound_enqueue(newpath,temp);

    unbound_print(temp);
    unbound_destroy(temp);
    free(temp);
}
*/
// gcc -fsanitize=address,undefined unbounded_queue.c