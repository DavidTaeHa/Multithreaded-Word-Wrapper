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
int Mthread = -1;
int Nthread = -1;

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

char* mystrsep(char** stringp, const char* delim)
{
  char* start = *stringp;
  char* p;

  p = (start != NULL) ? strpbrk(start, delim) : NULL;

  if (p == NULL)
  {
    *stringp = NULL;
  }
  else
  {
    *p = '\0';
    *stringp = p + 1;
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

void navDir(char *a, int b){
    if(DEBUG){
		printf("DB: %d - Navigate this folder: %s\n", b-1, a);
	}
    struct dirent *de;
    DIR *dr = opendir(".");

    if(dr == NULL){
        if(DEBUG){
			printf("DB: %d - Empty directory: %s\n", b, a);
		}
        return;
    }
    while ((de = readdir(dr)) != NULL){
        struct stat temp;
        if(stat(de->d_name, &temp) != -1 && de->d_name[0] != '.'){
            if(S_ISREG(temp.st_mode)){
                if(de->d_name[0] == '.'){
					return;
				}
				else if(strstr(de->d_name, "wrap.") == de->d_name){
					return;
				}
				else if(strstr(de->d_name, ".txt")){
					if(DEBUG){
						printf("DB: %d - Wrapping file: %s : %s\n", b, a, de->d_name);
					}
					int inText = open(de->d_name, O_RDONLY);
            		char *newFile = calloc(strlen(de->d_name) + 6, sizeof(char));
                    strcpy(newFile, "wrap.");
                    strcat(newFile, de->d_name);
                    if (DEBUG)
                    {
                        printf("DB: Wrapping file '%s' to '%s'\n", de->d_name, newFile);
                    }
                    int outText = open(newFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                    wrap_file(inText, outText);
                    close(inText);
                    close(outText);
                    free(newFile);
				}
            }
            else if(S_ISDIR(temp.st_mode)){
                char cwd[INPTSIZE];
                if(getcwd(cwd, sizeof(cwd)) != NULL){
                    if(DEBUG){
						printf("DB: Currently working dir: %s\n", cwd);
					}
                    chdir(de->d_name);
                    if(getcwd(cwd, sizeof(cwd)) != NULL){
                        if(DEBUG){
							printf("DB: Moving to dir: %s\n", cwd);
						}
                        navDir(de->d_name, b++);
                    }
                    chdir("..");
                }
            }
        }
    }
    closedir(dr);
    return;
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
    while (dir_queue->isEmpty == 0 || dir_queue->total_waiting < Mthread)
    {
        unbound_dequeue(&dir_name, dir_queue);
        if (dir_queue->isEmpty == 1 && dir_queue->total_waiting == Mthread)
        {
            printf("DIRECTORY QUEUE FINISHED!\n");
            break;
        }
        procDir(dir_name, dir_queue, file_queue);
        // unbound_print(dir_queue);
        // unbound_print(file_queue);
        //  printf("Worker Waiting: %d\n", dir_queue->total_waiting);
        //  printf("Worker Empty Status: %d\n", dir_queue->isEmpty);
    }
    finished++;
    if (finished == Mthread)
    {
        file_queue->dir_finished = 1;
        pthread_cond_broadcast(&file_queue->dequeue_ready);
        printf("ALL DIRECTORY THREADS FINISH!\n");
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
        printf("--------INPUT FILE: %s\n", file_name);
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
        printf("FILE LENGTH: %d\n", strlen(file_name));
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

        printf("%s\n", last);
        printf("--------OUTPUT: %s\n", temp4);

        int outText = open(temp4, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
        wrap_file(inText, outText);
        close(inText);
        close(outText);
    }
    printf("!!!!!!!!!!!!!!!!!!!!!!!!\n");
}

void threadInit(char *a){
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

    for(int i = 0; i < Mthread; i++){
        pthread_create(&tidsM[i], NULL, directory_worker, dir_queue);
    }
    for(int i = 0; i < Nthread; i++){
        pthread_create(&tidsN[i], NULL, file_worker, file_queue);
    }
    for(int i = 0; i < Mthread; i++){
        pthread_join(tidsM[i], NULL);
    }
    for(int i = 0; i < Nthread; i++){
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
    
    if (argc > 4 || argc < 2)
    {
        printf("Incorrect number of arguments\n");
        exit(EXIT_FAILURE);
    }
    
    // Parse argv[1] and assign 
    if(strncmp(argv[1], "-r", 2)){
      printf("Invalid argument 1");
      exit(EXIT_FAILURE);
    }
    // Accounts for -r by creating -r1,1
    if((argv[1] = "-r") && strlen(argv[1]) == 2){
        Mthread = 1;
		Nthread = 1;
    }
    else{
        char *token1, *token2;
        while((token1 = mystrsep(&argv[1], "-r")) !=  NULL){
            while((token2 = mystrsep(&token1, ",")) !=  NULL){
                if(Mthread == -1){
                    Mthread = atoi(token2);
                }
                else{
                    Nthread = atoi(token2);
                }
            }
        }
        // Accounts for -rN by creating -r1,N
        if(Nthread = -1){
            Nthread = Mthread;
            Mthread = 1;
        }
        if(DEBUG){
          if(Mthread){
            printf("DB: Value of M: %d\n", Mthread);
          }
          if(Nthread){
            printf("DB: Value of N: %d\n", Nthread);
          }
        }
    }
    
    //Checks if argv[2] is a positive number
    if(!isdigit((char) argv[2][0]))
    {
        printf("Invalid width value.\n");
        exit(EXIT_FAILURE);
    }
    columns = atoi(argv[2]);

    struct stat temp;
    // If second argument is an existing file or directory
    if (stat(argv[3], &temp) != -1)
    {
        // Second argument is a file that exists
        if (S_ISREG(temp.st_mode))
        {
            if (DEBUG)
            {
                printf("\nFile '%s' wrapped to STDOUT\n", argv[3]);
            }
            int inText = open(argv[3], O_RDONLY);
            wrap_file(inText, 1);
            close(inText);
        }
        // Second argument is a directory that exists
        else if (S_ISDIR(temp.st_mode))
        {
            if (DEBUG)
            {
                printf("\nWrapping files in directory '%s'\n", argv[3]);
            }
            struct dirent *f;
            DIR *fd = opendir(argv[3]);
            chdir(argv[3]);

			if(fd == NULL){
				if(DEBUG){
					printf("DB: Empty directory");
					return exitCode;
				}
			}			
			// Project 3 Part 1: M = 1, N = 1
			if(Mthread = 1 && Nthread == 1)
			{
				if(DEBUG){
					printf("DB: M = %d, N = %d", Mthread, Nthread);
				}
				navDir(f->d_name, 1);
			}
			// PROJECT 3 Part 2: M != 1 and/or N != 1
			else{
                threadInit(argv[3]);
			}
            closedir(fd);
        }
    }
    // If second arguments file name does not exist read from STDIN
    else if (argc == 3)
    {
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

        //Reads from stdin and prints out to a temp file
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
        else{
            perror(tempName);
            exit(EXIT_FAILURE);
        }
        // Remove temporary file as it is no longer needed and should not exist.
        close(outText);
        remove(tempName);
        free(tempName);
    }
    else
    {
        if (argc > 4 || argc < 2)
        {
            printf("Error: Not enough arguments");
        }
        else if ((stat(argv[3], &temp) == -1))
        {
            perror(argv[3]);
        }
        exitCode = EXIT_FAILURE;
    }
    return exitCode;
}