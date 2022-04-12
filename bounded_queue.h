#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>

#ifndef DEBUG
#define DEBUG 1
#endif

#define FILEPATH 4096
#define QUEUESIZE 16

struct bounded_queue
{
    char **names;
    int start, stop;
    int full;
    int empty;
    int wait_status;
    int capacity;
    pthread_mutex_t lock;
    pthread_cond_t enqueue_ready, dequeue_ready;
};

int queue_init(struct bounded_queue *q);
int queue_destroy(struct bounded_queue *q);
int enqueue(char* n, struct bounded_queue *q);
int dequeue(char** n, struct bounded_queue *q);
void print_queue(struct bounded_queue *q);