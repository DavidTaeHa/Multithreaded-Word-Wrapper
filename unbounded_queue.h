#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>

#ifndef DEBUG
#define DEBUG 1
#endif

#define FILEPATH 4096

// struct defines queue of different names
struct unbounded_queue
{
    char **names;
    int start, stop;
    int isEmpty;
    pthread_mutex_t lock;
};

int unbound_init(struct unbounded_queue *q);
int unbound_destroy(struct unbounded_queue *q);
int unbound_enqueue(char* n, struct unbounded_queue *q);
int unbound_dequeue(char** n, struct unbounded_queue *q);
void unbound_print(struct unbounded_queue *q);