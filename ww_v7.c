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
#include "file_queue.h"

struct unbounded_queue *dir_queue;
struct file_queue *file_queue;

#ifndef DEBUG
#define DEBUG 0
#endif

// Set to 1, Best at 4.
#define BUFFSIZE 1
// Choosing large value for user input
#define INPTSIZE 4096

static int exitCode = EXIT_SUCCESS;
int columns = 1;
int finished = 0;
int Mthread = 0;
int Nthread = 0;

char *mystrsep(char **strp, const char *d)
{
    char *start = *strp;
    char *p;

    p = (start != NULL) ? strpbrk(start, d) : NULL;

    if (p == NULL)
    {
        *strp = NULL;
    }
    else
    {
        *p = '\0';
        *strp = p + 1;
    }

    return start;
}

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

void procDir(char *a, struct unbounded_queue *q, struct file_queue *r)
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

        // char *newfile = malloc(strlen(newpath) + 6);

        // && strstr(de->d_name, "wrap.") != de->d_name && strstr(de->d_name, ".txt") && de->d_name[0] != '.'
        if (stat(newpath, &temp) != -1 && de->d_name[0] != '.')
        {
            if (S_ISREG(temp.st_mode) && strstr(de->d_name, "wrap.") != de->d_name && strstr(de->d_name, ".txt"))
            {
                char *wrap = (char *)malloc(flen + 6);
                strcpy(wrap, "wrap.");
                strcat(wrap, file);
                int wlen = strlen(wrap);
                char *newfile = malloc(plen + wlen + 2);
                memcpy(newfile, path, plen);
                newfile[plen] = '/';
                memcpy(newfile + plen + 1, wrap, wlen + 1);
                free(wrap);

                int val = file_enqueue(newpath, newfile, r);
                if (val == 1)
                {
                    free(newpath);
                    free(newfile);
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
    while (unbound_dequeue(&dir_name, dir_queue) == 1)
    {
        /*
        if (dir_queue->isEmpty == 1 && dir_queue->total_waiting == Mthread)
        {
            printf("DIRECTORY QUEUE FINISHED!\n");
            break;
        }
        */
        procDir(dir_name, dir_queue, file_queue);
        // unbound_print(dir_queue);
        // unbound_print(file_queue);
        // printf("Worker Waiting: %d\n", dir_queue->total_waiting);
        // printf("Worker Empty Status: %d\n", dir_queue->isEmpty);
        // printf("Worker closed Status: %d\n", dir_queue->closed);
    }
    /*
    finished++;
    printf("Finished: %d\n", finished);
    if (finished == Mthread)
    {
        file_queue->dir_finished = 1;
        pthread_cond_broadcast(&file_queue->dequeue_ready);
        printf("ALL DIRECTORY THREADS FINISH!\n");
    }
    */
}

void *file_worker(void *args)
{
    char *file_name;
    char *wrap_name;
    while (file_queue->dir_finished == 0 || file_queue->isEmpty == 0)
    {
        file_dequeue(&file_name, &wrap_name, file_queue);
        if (file_queue->dir_finished == 1 && file_queue->isEmpty == 1)
        {
            pthread_cond_broadcast(&file_queue->dequeue_ready);
            break;
        }
        printf("--------INPUT FILE: %s\n", file_name);
        int inText = open(file_name, O_RDONLY);

        printf("--------OUTPUT: %s\n", wrap_name);

        int outText = open(wrap_name, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        wrap_file(inText, outText);
        close(inText);
        close(outText);
    }
    printf("!!!!!!!!!!!!!!!!!!!!!!!!\n");
}

void threadInit(char *a)
{
    pthread_t tidsM[Mthread];
    pthread_t tidsN[Nthread];
    dir_queue = malloc(sizeof(struct unbounded_queue));
    file_queue = malloc(sizeof(struct file_queue));
    unbound_init(dir_queue, Mthread);
    file_init(file_queue);

    char *path = ".";
    int plen = strlen(path);
    int flen = strlen(a);
    char *newpath = malloc(plen + flen + 2);
    memcpy(newpath, path, plen);
    newpath[plen] = '/';
    memcpy(newpath + plen + 1, a, flen + 1);

    unbound_enqueue(newpath, dir_queue);

    for (int i = 0; i < Mthread; i++)
    {
        pthread_create(&tidsM[i], NULL, directory_worker, dir_queue);
    }
    for (int i = 0; i < Nthread; i++)
    {
        pthread_create(&tidsN[i], NULL, file_worker, file_queue);
    }
    for (int i = 0; i < Mthread; i++)
    {
        pthread_join(tidsM[i], NULL);
    }

    file_queue->dir_finished = 1;
    pthread_cond_broadcast(&file_queue->dequeue_ready);
    printf("ALL DIRECTORY THREADS FINISHED...\n");

    for (int i = 0; i < Nthread; i++)
    {
        pthread_join(tidsN[i], NULL);
    }

    unbound_print(dir_queue);
    unbound_destroy(dir_queue);
    free(dir_queue);
    file_print(file_queue);
    file_destroy(file_queue);
    free(file_queue);
    return;
}

int main(int argc, char **argv)
{
    // Check for absolute minimal arguments required
    if (argc < 2)
    {
        printf("Incorrect number of arguments\n");
        exit(EXIT_FAILURE);
    }

    int checker;
    // Parse argv[1] and assign
    if (strncmp(argv[1], "-r", 2))
    {
        checker = 0;
    }
    else
    {
        checker = 1;
    }

    if (checker == 1)
    {
        // Check for minimal arguments required in the event of establishing multithreading
        if(argc <= 3){
            printf("Incorrect number of arguments\n");
            exit(EXIT_FAILURE);
        }
        // Accounts for -r by creating -r1,1
        if (!strncmp(argv[1], "-r", 3))
        {
            if (DEBUG)
            {
                printf("Argument inputed as -r\n");
            }
            Mthread = Nthread = 1;
        }
        else
        {
            char *token1, *token2;
            while ((token1 = mystrsep(&argv[1], "-r")) != NULL)
            {
                while ((token2 = mystrsep(&token1, ",")) != NULL)
                {
                    if (!Mthread)
                    {
                        Mthread = atoi(token2);
                    }
                    else
                    {
                        Nthread = atoi(token2);
                    }
                }
            }
            // Accounts for -rN by creating -r1,N
            if (Nthread == 0)
            {
                Nthread = Mthread;
                Mthread = 1;
            }
            if (DEBUG)
            {
                if (Mthread)
                {
                    printf("DB: Value of M: %d\n", Mthread);
                }
                if (Nthread)
                {
                    printf("DB: Value of N: %d\n", Nthread);
                }
            }
        }
        if (Nthread == 0)
        {
            Nthread = 1;
        }
    }

    if (checker == 0)
    {
        // Checks if argv[1] is a positive number
        if (!isdigit((char)argv[1][0]))
        {
            printf("Invalid argument at positon 1:\n\tRequires positive width value or establish multithreading using forms '-r', '-rN', '-rM,N'.\n");
            exit(EXIT_FAILURE);
        }
        columns = atoi(argv[1]);
    }
    else
    {
        // Checks if argv[2] is a positive number
        if (!isdigit((char)argv[2][0]))
        {
            printf("Invalid width value.\n");
            exit(EXIT_FAILURE);
        }
        columns = atoi(argv[2]);
    }

    if (checker == 0)
    {
        if (argc == 2)
        {
            struct stat temp;
            char *userStr = malloc(sizeof(char) * INPTSIZE);

            // Creating a temporary file
            char *tempName = malloc(sizeof(char) * 11);
            char *tempNum = malloc(sizeof(char) * 6);
            strcpy(tempName, "temp");
            srand(time(NULL));
            int nameNum = rand() % (99999 - 10000) + 10000;
            sprintf(tempNum, "%d", nameNum);
            strcat(tempName, tempNum);
            strcat(tempName, ".txt");
            free(tempNum);

            // Reads from stdin and prints out to a temp file
            int outText = open(tempName, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
            read(0, userStr, INPTSIZE);
            userStr[strlen(userStr)] = '\0';
            write(outText, userStr, strlen(userStr));
            free(userStr);

            // If the file exists proceed
            if ((stat(tempName, &temp) != -1))
            {
                if (DEBUG)
                {
                    printf("\nTemporary file '%s' wrapped to STDOUT\n", tempName);
                }
                int inText = open(tempName, O_RDONLY);
                 wrap_file(inText, 1);
                close(inText);
            }
            else
            {
                perror(tempName);
                exit(EXIT_FAILURE);
            }
            // Remove temporary file as it is no longer needed and should not exist.
            close(outText);
            remove(tempName);
            free(tempName);
        }
        else{
            for(int currArg = 2; currArg < argc + 1; currArg++){
                struct stat temp;
                // If second argument is an existing file or directory
                if (stat(argv[currArg], &temp) != -1)
                {
                    // Second argument is a file that exists
                    if (S_ISREG(temp.st_mode))
                    {
                        if (DEBUG)
                        {
                            printf("\nFile '%s' wrapped\n", argv[currArg]);
                        }
                        char topDir[INPTSIZE];
                        getcwd(topDir, INPTSIZE);
                        char *tempArg = calloc(strlen(argv[currArg]), sizeof(char));
                        tempArg = strcpy(tempArg, argv[currArg]);
                        char *newDir = calloc(strlen(tempArg), sizeof(char));
                        char *fToken, fTokenCpy[INPTSIZE];
                        int zSlash = 0, zSkip = 0;
                        while((fToken = mystrsep(&tempArg, "/")) != NULL){
                            if(zSkip == 1){
                                if(zSlash == 1){
                                    strcat(newDir, "/");
                                }
                                strcat(newDir, fTokenCpy);
                                zSlash = 1;
                            }
                            strcpy(fTokenCpy, fToken);
                            zSkip = 1;
                        }
                        chdir(newDir);
                        char *newFile = calloc(strlen(fTokenCpy) + 6, sizeof(char));
                        strcat(newFile, "wrap.");
                        strcat(newFile, fTokenCpy);
                        int inText = open(fTokenCpy, O_RDONLY);
                        int outText = open(newFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                        wrap_file(inText, outText);
                        close(inText);
                        close(outText);
                        free(newDir);
                        free(newFile);
                        chdir(topDir);
                    }
                    // Second argument is a directory that exists
                    else if (S_ISDIR(temp.st_mode))
                    {
                        if (DEBUG)
                        {
                            printf("\nWrapping files in directory '%s'\n", argv[currArg]);
                        }
                        struct dirent *f;
                        DIR *fd = opendir(argv[currArg]);
                        char topDir[INPTSIZE];
                        getcwd(topDir, INPTSIZE);
                        chdir(argv[currArg]);
                        int count = 1;
                        while ((f = readdir(fd)) != NULL)
                        {
                            if (f->d_name[0] == '.')
                            {
                                printf("Skipping: '%s'\n", f->d_name);
                            }
                            else if (strstr(f->d_name, "wrap.") == f->d_name)
                            {
                                printf("File to overwrite: '%s'\n", f->d_name);
                            }
                            else if (strstr(f->d_name, ".txt"))
                            {
                                int inText = open(f->d_name, O_RDONLY);
                                char *newFile = calloc(strlen(f->d_name) + 6, sizeof(char));
                                strcpy(newFile, "wrap.");
                                strcat(newFile, f->d_name);
                                if (DEBUG)
                                {
                                    printf("\n%d: Wrapping file '%s' to '%s'\n", count, f->d_name, newFile);
                                }
                                int outText = open(newFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                                wrap_file(inText, outText);
                                close(inText);
                                close(outText);
                                free(newFile);
                                count++;
                            }
                        }
                        chdir(topDir);
                        closedir(fd);
                    }
                }
            }
        }
    }
    else
    {
        if (DEBUG)
        {
            printf("DB: M = %d, N = %d\n", Mthread, Nthread);
        }
        printf("DB: M = %d, N = %d\n", Mthread, Nthread);
        threadInit(argv[3]);
    }
    return exitCode;
}

// gcc ww.c unbounded_queue.c file_queue.c