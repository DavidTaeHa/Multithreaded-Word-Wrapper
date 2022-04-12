#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <limits.h>

#ifndef DEBUG
#define DEBUG 1
#endif

#define FILEPATH 4096

int QUEUESIZE = 2;

// struct defines queue of different names
struct queue
{
    char **names;
    int start, stop;
    int isEmpty;
};

//initializes the queue
int queue_init(struct queue *q)
{
    q->start = 0;
    q->stop = 0;
    q->isEmpty = 0;
    q->names = malloc(sizeof(char *) * QUEUESIZE);
    for (int i = 0; i < QUEUESIZE; i++)
    {
        q->names[i] = malloc(sizeof(char) * FILEPATH);
        q->names[i] = NULL;
    }
    return 0;
}

//Adds name to the queue
int enqueue(char *n, struct queue *q)
{
    if (DEBUG)
        printf("Enqueueing \'%s\'...\n", n);

    // Increases size of queue
    if (q->stop == QUEUESIZE)
    {
        if (DEBUG)
            printf("Increasing size...\n");

        //Doubles size of queue
        q->names = realloc(q->names, 2 * QUEUESIZE * sizeof(char *));
        for (int i = QUEUESIZE; i < (QUEUESIZE * 2); i++)
        {
            q->names[i] = malloc(sizeof(char) * FILEPATH);
            q->names[i] = NULL;
        }

        // Add the new bigger queue to the struct
        QUEUESIZE *= 2;
    }

    // Adds item to the queue and increments end of queue
    q->names[q->stop] = n;
    q->stop++;
    printf("%d\n", q->stop);

    return 0;
}

//Dequeues names from the queue
int dequeue(char **n, struct queue *q)
{
    if (DEBUG)
        printf("Dequeuing...\n");

    //Checks if queue is empty
    if(q->stop != q->start){
        q->isEmpty = 0;
    }

    if (q->isEmpty)
    {
        printf("Queue is empty...\n");
    }

    // Dequeues item and increments start of queue
    *n = q->names[q->start];
    q->names[q->start] = NULL;
    q->start++;

    // Checks if queue is empty
    if (q->stop == q->start)
    {
        q->isEmpty = 1;
    }
    return 0;
}

// Prints out all elements within the queue for testing
void print_queue(struct queue *q)
{
    for (int i = 0; i < QUEUESIZE; i++)
    {
        if(q->names[i] == NULL){
            printf("(EMPTY)\n");
        }
        else{
            printf("%s\n", q->names[i]);
        }
    }
}

// testing out queue
int main()
{
    struct queue *temp = malloc(sizeof(struct queue));
    queue_init(temp);
    enqueue("I believe I can fly", temp);
    enqueue("I believe I can touch the sky", temp);
    //enqueue("I think about it every night and day", temp);
    char *phrase;
    char *phrase2;
    //dequeue(&phrase, temp);
    //dequeue(&phrase2, temp);
    print_queue(temp);
    return 0;
}