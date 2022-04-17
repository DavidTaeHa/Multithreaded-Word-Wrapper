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
#include "bounded_queue.h"
#include "unbounded_queue.h"

#ifndef DEBUG
#define DEBUG 0
#endif

// Set to 1, Best at 4.
#define BUFFSIZE 1
// Choosing large value for user input
#define INPTSIZE 4096

static int exitCode = EXIT_SUCCESS;

struct unbounded_queue *dir_queue;
struct unbounded_queue *file_queue;
int thread_count = 5;
int columns = 15;

void wrap_file(int file_in, int file_out, int columns)
{
    int length = BUFFSIZE, bytes = 0, brite = 0, last = 0, nLin = 0, nPar = 0, n = 0, fLen;
    char *buf = malloc(sizeof(char) * BUFFSIZE);
    char *word = calloc(length, sizeof(char));
    if (DEBUG)
    {
        printf("Debugging:\n");
    }
    while ((bytes = read(file_in, buf, BUFFSIZE)) > 0)
    {
        for (size_t i = 0; i < bytes; ++i)
        {
            // Read char in buffer
            if (DEBUG)
            {
                printf("%ld: '%c'\n", i, buf[i]);
            }
            if (isspace(buf[i]))
            {
                if (n)
                {
                    fLen = *&brite + n;
                    if (*&brite != 0)
                    {
                        fLen++;
                    }
                    // Word will exceed maximum width, requires separate line
                    if (fLen > columns)
                    {
                        write(file_out, "\n", 1);
                        *&brite = 0;
                    }
                    // Adds space before next word on same line
                    if (brite != 0 && !last)
                    {
                        *&brite += write(file_out, " ", 1);
                    }
                    *&brite += write(file_out, &word[0], n);
                    // Written word exceeded given width
                    if (brite > columns)
                    {
                        if (DEBUG)
                        {
                            printf("    Error: Word '%s' has exceeded wrapping parameters\n", word);
                        }
                        exitCode = EXIT_FAILURE;
                    }
                    if (DEBUG)
                    {
                        printf("        Checked %d Bytes of the word '%s'\n", brite, word);
                        printf("        Added %d Bytes of the word '%s'\n", n, word);
                    }
                    free(word);
                    word = calloc(length, sizeof(char));
                    n = 0;
                }
                if (buf[i] == 10)
                {
                    if (!nLin)
                    {
                        if (DEBUG)
                        {
                            printf("        New Line\n");
                        }
                        nLin = 1;
                    }
                    else if (!nPar)
                    {
                        if (DEBUG)
                        {
                            printf("        Paragraph Ended\n");
                        }
                        write(file_out, "\n\n", 2);
                        brite = nLin = 0;
                        nPar = 1;
                    }
                }
                continue;
            }
            else
            {
                if (n == (length - 1))
                {
                    word = realloc(word, sizeof(char) * (length *= 2));
                }
                word[n++] = buf[i];
                if (DEBUG)
                {
                    printf("    Current Word: '%s'\n", word);
                }
                nLin = nPar = 0;
            }
        }
        free(buf);
        buf = malloc(sizeof(char) * BUFFSIZE);
    }
    fLen = *&brite + n;
    if (*&brite != 0)
    {
        fLen++;
    }
    // Word will exceed maximum width, requires separate line
    if (fLen > columns)
    {
        write(file_out, "\n", 1);
        *&brite = 0;
    }
    // Adds space before final word
    if (brite != 0 && !last)
    {
        *&brite += write(file_out, " ", 1);
    }
    *&brite += write(file_out, &word[0], n);
    if (DEBUG)
    {
        printf("        Checked %d Bytes of the word '%s'\n", brite, word);
        printf("        Added %d Bytes of the word '%s'\n", n, word);
    }
    // Final written word exceeded given width
    if (brite > columns)
    {
        if (DEBUG)
        {
            printf("    Error: Word '%s' has exceeded wrapping parameters\n", word);
        }
        exitCode = EXIT_FAILURE;
    }
    write(file_out, "\n", 1);
    free(word);
    free(buf);
}

void procDir(char *a, int columns, struct unbounded_queue *qDir, struct unbounded_queue *qFile)
{
    struct dirent *de;
    DIR *dr = opendir(".");
    if (dr == NULL)
    {
        closedir(dr);
        return;
    }
    while ((de = readdir(dr)) != NULL)
    {
        struct stat temp;
        if (stat(de->d_name, &temp) != -1 && de->d_name[0] != '.')
        {
            if (S_ISREG(temp.st_mode))
            {
                if (de->d_name[0] == '.')
                {
                    return;
                }
                else if (strstr(de->d_name, "wrap.") == de->d_name)
                {
                    return;
                }
                else if (strstr(de->d_name, ".txt"))
                {
                    // Here it will make the call to FileProcessing Worker qFile, queuing de->d_name
                    printf("File to wrap: %s\n", de->d_name);
                    // unbound_enqueue(de->d_name, file_queue);
                    return;
                }
            }
        }
    }
    closedir(dr);
    return;
}
// just print for now

// This version only traverses through current given directory
void navDir(char *a, struct unbounded_queue *q, struct unbounded_queue *r)
{ // example input: ./foldera
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
                // printf("%s\n", newpath);
                int val = unbound_enqueue(newpath, r);
                if (val == 1)
                {
                    free(newpath);
                }
                // free(newpath);
                // NOTE: add code to add the text file path to the file queue and remove free statement later
            }
            else if (S_ISDIR(temp.st_mode))
            {
                // NOTE: add code to add the directory file path to the  directory queue
                int val = unbound_enqueue(newpath, q);
                if (val == 1)
                {
                    free(newpath);
                }
                // printf("%s\n", newpath);
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
    char *dir_name;
    // Work on the directory queue while it is not empty or the number of waiting threads is less than total threads
    while (dir_queue->isEmpty == 0 || dir_queue->total_waiting < thread_count)
    {
        // directory queue is empty and number of total waiting threads is about to go up to max counts
        /*
        if (dir_queue->isEmpty == 1 || dir_queue->total_waiting == thread_count)
        {
            pthread_cond_broadcast(&dir_queue->dequeue_ready);
            pthread_mutex_unlock(&dir_queue->lock);
            break;
        }
        */
        unbound_dequeue(&dir_name, dir_queue);
        if (dir_queue->isEmpty == 1 && dir_queue->total_waiting == thread_count)
        {
            printf("DIRECTORY QUEUE FINISHED!\n");
            pthread_cond_broadcast(&dir_queue->dequeue_ready);
            break;
        }
        navDir(dir_name, dir_queue, file_queue);
        //unbound_print(dir_queue);
        unbound_print(file_queue);
        // printf("Worker Waiting: %d\n", dir_queue->total_waiting);
        // printf("Worker Empty Status: %d\n", dir_queue->isEmpty);
    }
    printf("THREAD FINISH!\n");
}

void *file_worker(void *args)
{
    char *file_name;
    while (file_queue->dir_finished == 0 || file_queue->isEmpty == 0)
    {
        unbound_dequeue(&file_name, file_queue);
        // int inText = open(file_name, O_RDONLY);
        printf("--------INPUT FILE: %s\n", file_name);

        // int outText = open(result_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        // wrap_file(inText, outText, 15);
        // close(inText);
        // close(outText);
    }
    printf("!!!!!!!!!!!!!!!!!!!!!!!!\n");
}

int main()
{
    pthread_t pid, pid2, pid3, pid4, pid5, pid6, pid7, pid8, pid9, pid10;
    dir_queue = malloc(sizeof(struct unbounded_queue));
    file_queue = malloc(sizeof(struct unbounded_queue));
    unbound_init(dir_queue, thread_count);
    unbound_init(file_queue, 1);

    char *path = ".";
    char *file = "foldera";
    int plen = strlen(path);
    int flen = strlen(file);
    char *newpath = malloc(plen + flen + 2);
    memcpy(newpath, path, plen);
    newpath[plen] = '/';
    memcpy(newpath + plen + 1, file, flen + 1);

    unbound_enqueue(newpath, dir_queue);

    pthread_create(&pid, NULL, directory_worker, NULL);
    pthread_create(&pid2, NULL, directory_worker, NULL);
    pthread_create(&pid3, NULL, directory_worker, NULL);
    pthread_create(&pid4, NULL, directory_worker, NULL);
    pthread_create(&pid5, NULL, directory_worker, NULL);
    //pthread_create(&pid6, NULL, directory_worker, NULL);
    //pthread_create(&pid7, NULL, directory_worker, NULL);
    //pthread_create(&pid8, NULL, directory_worker, NULL);
    //pthread_create(&pid9, NULL, directory_worker, NULL);
    //pthread_create(&pid10, NULL, directory_worker, NULL);

    pthread_join(pid, NULL);
    pthread_join(pid2, NULL);
    pthread_join(pid3, NULL);
    pthread_join(pid4, NULL);
    pthread_join(pid5, NULL);
    //pthread_join(pid6,NULL);
    //pthread_join(pid7,NULL);
    //pthread_join(pid8,NULL);
    //pthread_join(pid9,NULL);
    //pthread_join(pid10,NULL);

    // Might be error here, some iterations do not pass this line
    unbound_print(dir_queue);
    unbound_destroy(dir_queue);
    free(dir_queue);
    unbound_print(file_queue);
    unbound_destroy(file_queue);
    free(file_queue);
    free(newpath);
    return 0;
}
// gcc -fsanitize=address,undefined directory_traverse.c unbounded_queue.c
//  gcc directory_traverse.c unbounded_queue.c bounded_queue.c