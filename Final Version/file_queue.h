#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>

#ifndef DEBUG
#define DEBUG 0
#endif

#define FILEPATH 4096

// struct defines queue of different names
struct file_queue
{
    char **names;
    char **wraps;
    int start, stop;
    int isEmpty;
    int dir_finished;
    pthread_mutex_t lock ,lock2;
    pthread_cond_t dequeue_ready;
};

int file_init(struct file_queue *q);
int file_destroy(struct file_queue *q);
int file_enqueue(char *n, char *m, struct file_queue *q);
int file_dequeue(char **n, char **m, struct file_queue *q);
void file_print(struct file_queue *q);