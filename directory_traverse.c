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
#include "file_queue.h"
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
struct file_queue *file_queue;
int dir_thread = 0;
int file_thread = 0;
int columns = 15;
int finished = 0;
int active_threads = 0;

void wrap_file(int file_in, int file_out)
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

// This version only traverses through current given directory
void navDir(char *a, struct unbounded_queue *q, struct file_queue *r)
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
                int val = file_enqueue(newpath, r);
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
    while (dir_queue->isEmpty == 0 || dir_queue->total_waiting < dir_thread)
    {
        unbound_dequeue(&dir_name, dir_queue);
        if (dir_queue->isEmpty == 1 && dir_queue->total_waiting == dir_thread)
        {
            printf("One Directory Thread Finished...\n");
            break;
        }
        navDir(dir_name, dir_queue, file_queue);
        //file_print(file_queue);
    }
    finished++;
    if (finished == dir_thread)
    {
        file_queue->dir_finished = 1;
        pthread_cond_broadcast(&file_queue->dequeue_ready);
        printf("All Directory Threads Finished!\n");
    }
}

void *file_worker(void *args)
{
    char *file_name;
    while (file_queue->dir_finished == 0 || file_queue->isEmpty == 0)
    {
        file_dequeue(&file_name, file_queue);
        if (file_queue->dir_finished == 1 && file_queue->isEmpty == 1)
        {
            pthread_cond_broadcast(&file_queue->dequeue_ready);
            break;
        }
        printf("Wrapping %s...\n", file_name);
        int inText = open(file_name, O_RDONLY);

        char *token;
        char *last;
        char temp[4096];
        char temp2[4096];
        char filename_cpy[4096];
        memset(temp,0,sizeof(temp));
        memset(temp2,0,sizeof(temp2));
        memset(filename_cpy,0,sizeof(filename_cpy));

        strcpy(filename_cpy, file_name);
        token = strtok(filename_cpy, "/");

        strcat(temp, token);
        while (token != NULL)
        {
            strcat(temp, "/");
            token = strtok(NULL, "/");
            if (token != NULL)
            {
                strcat(temp, token);
                last = token;
            }
        }

        token = strtok(temp, "/");
        strcat(temp2, token);
        while (token != NULL)
        {
            token = strtok(NULL, "/");
            if (token != NULL)
            {
                strcat(temp2, "/");
                if (strcmp(last, token) != 0)
                {
                    strcat(temp2, token);
                }
            }
        }

        char temp3[4096];
        char temp4[4096];
        memset(temp3,0,sizeof(temp3));
        memset(temp4,0,sizeof(temp4));

        strcat(temp3, "wrap.");
        strcat(temp3, last);
        strcat(temp4, temp2);
        strcat(temp4, temp3);

        int outText = open(temp4, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        wrap_file(inText, outText);
        close(inText);
        close(outText);
    }
}

int main()
{
    int M = 10;
    int N = 1;
    dir_thread = M;
    file_thread = N;
    columns = 80;

    pthread_t tidsM[dir_thread];
    pthread_t tidsN[file_thread];
    dir_queue = malloc(sizeof(struct unbounded_queue));
    file_queue = malloc(sizeof(struct file_queue));
    unbound_init(dir_queue, dir_thread);
    file_init(file_queue);

    char *path = ".";
    char *file = "foldera";
    int plen = strlen(path);
    int flen = strlen(file);
    char *newpath = malloc(plen + flen + 2);
    memcpy(newpath, path, plen);
    newpath[plen] = '/';
    memcpy(newpath + plen + 1, file, flen + 1);

    unbound_enqueue(newpath, dir_queue);

    for(int i = 0; i < dir_thread; i++){
        pthread_create(&tidsM[i], NULL, directory_worker, dir_queue);
    }
    for(int i = 0; i < file_thread; i++){
        pthread_create(&tidsN[i], NULL, file_worker, file_queue);
    }
    for(int i = 0; i < dir_thread; i++){
        pthread_join(tidsM[i], NULL);
    }
    for(int i = 0; i < file_thread; i++){
        pthread_join(tidsN[i], NULL);
    }

    // Might be error here, some iterations do not pass this line
    unbound_print(dir_queue);
    unbound_destroy(dir_queue);
    free(dir_queue);
    file_print(file_queue);
    file_destroy(file_queue);
    free(file_queue);
    printf("Finished Wrapping...\n");
    return 0;
}
// gcc -fsanitize=address,undefined directory_traverse.c unbounded_queue.c file_queue.c
//  gcc directory_traverse.c unbounded_queue.c file_queue.c