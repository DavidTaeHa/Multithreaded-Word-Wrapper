#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include "unbounded_queue.h"
#include "bounded_queue.h"

struct unbounded_queue *dir_queue;

// This version of the method traverse recursively through all of the subdirectories
/*
void navDir_whole(char *a)
{
    struct dirent *de;
    DIR *dr = opendir(a);

    if (dr == NULL)
    {
        printf("Empty directory: %s\n", a);
        return;
    }
    while ((de = readdir(dr)) != NULL)
    {
        struct stat temp;

        // Create filepath
        char *path = a;
        char *file = de->d_name;
        int plen = strlen(path);
        int flen = strlen(file);
        char *newpath = malloc(plen + flen + 2);
        memcpy(newpath, path, plen);
        newpath[plen] = '/';
        memcpy(newpath + plen + 1, file, flen + 1);

        if (stat(newpath, &temp) != -1 && de->d_name[0] != '.')
        {
            if(S_ISREG(temp.st_mode) && strstr(de->d_name, "wrap.") != de->d_name && strstr(de->d_name, ".txt")){
                printf("%s\n", newpath);
                //NOTE: add code to add the text file path to the queue
            }
            else if (S_ISDIR(temp.st_mode))
            {
                //NOTE: add code to add the directory file path to the queue
                printf("%s\n", newpath);
                navDir_whole(newpath);
            }
        }
    }
    closedir(dr);
    return;
}
*/

// This version only traverses through current given directory
void navDir(char *a, struct unbounded_queue *q)
{
    struct dirent *de;
    DIR *dr = opendir(a);

    if (dr == NULL)
    {
        printf("Empty directory: %s\n", a);
        return;
    }
    while ((de = readdir(dr)) != NULL)
    {
        struct stat temp;

        // Create filepath
        char *path = a;
        char *file = de->d_name;
        int plen = strlen(path);
        int flen = strlen(file);
        char *newpath = malloc(plen + flen + 2);
        memcpy(newpath, path, plen);
        newpath[plen] = '/';
        memcpy(newpath + plen + 1, file, flen + 1);

        // && strstr(de->d_name, "wrap.") != de->d_name && strstr(de->d_name, ".txt") && de->d_name[0] != '.'
        if (stat(newpath, &temp) != -1 && de->d_name[0] != '.')
        {
            if (S_ISREG(temp.st_mode) && strstr(de->d_name, "wrap.") != de->d_name && strstr(de->d_name, ".txt"))
            {
                printf("%s\n", newpath);
                free(newpath);
                // NOTE: add code to add the text file path to the file queue and remove free statement later
            }
            else if (S_ISDIR(temp.st_mode))
            {
                // NOTE: add code to add the directory file path to the  directory queue
                unbound_enqueue(newpath, q);
                printf("%s\n", newpath);
            }
            else
            {
                free(newpath);
            }
        }
        else
        {
            free(newpath);
        }
    }
    // for some reason, closedir causes complications
    closedir(dr);
}

void *directory_worker(void *args)
{
    struct unbounded_queue *temp = (struct unbounded_queue *)args;
    char *dir_name;
    unbound_dequeue(&dir_name, dir_queue);
    printf("DEQUEUE: %s\n", dir_name);
    navDir(dir_name, dir_queue);
}

int main()
{
    pthread_t pid, pid2;
    dir_queue = malloc(sizeof(struct unbounded_queue));
    unbound_init(dir_queue);
    char *dir = "./test_folder";
    unbound_enqueue(dir, dir_queue);
    // navDir(dir, dir_queue);

    pthread_create(&pid, NULL, directory_worker, dir_queue);
    pthread_join(pid, NULL);

    unbound_print(dir_queue);

    pthread_create(&pid2, NULL, directory_worker, dir_queue);
    pthread_join(pid2, NULL);
    // Might be error here, some iterations do not pass this line
    unbound_print(dir_queue);
    unbound_destroy(dir_queue);
    free(dir_queue);
    printf("Boom shaka laka\n");
    return 0;
}
// gcc directory_traverse.c unbounded_queue.c bounded_queue.c