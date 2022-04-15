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

#ifndef DEBUG
#define DEBUG 0
#endif

// Set to 1, Best at 4.
#define BUFFSIZE 1
// Choosing large value for user input
#define INPTSIZE 4096

static int exitCode = EXIT_SUCCESS;

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

void navDir(char *a, int b, int c){
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
                    wrap_file(inText, outText, c);
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
                        navDir(de->d_name, b++, c);
                    }
                    chdir("..");
                }
            }
        }
    }
    closedir(dr);
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
    int digM, digN;
    digM = digN = -1;
    // Accounts for -r by creating -r1,1
    if((argv[1] = "-r") && strlen(argv[1]) == 2){
        digM = 1;
		digN = 1;
    }
    else{
        char *token1, *token2;
        while((token1 = strsep(&argv[1], "-r")) !=  NULL){
            while((token2 = strsep(&token1, ",")) !=  NULL){
                if(!digM){
                    digM = atoi(token2);
                }
                else{
                    digN = atoi(token2);
                }
            }
        }
        // Accounts for -rN by creating -r1,N
        if(digN = -1){
            digN = digM;
            digM = 1;
        }
        if(DEBUG){
          if(digM){
            printf("DB: Value of M: %d\n", digM);
          }
          if(digN){
            printf("DB: Value of N: %d\n", digN);
          }
        }
    }
    
    //Checks if argv[2] is a positive number
    if(!isdigit((char) argv[2][0]))
    {
        printf("Invalid width value.\n");
        exit(EXIT_FAILURE);
    }

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
            wrap_file(inText, 1, atoi(argv[2]));
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
			if(digM = 1 && digN == 1)
			{
				if(DEBUG){
					printf("DB: M = %d, N = %d", digM, digN);
				}
				navDir(f->d_name, 1, atoi(argv[2]));
			}
			// PROJECT 3 Part 2: M = 1, N != 1
			else if(digM == 1){
				if(DEBUG){
					printf("DB: M = %d, N = %d", digM, digN);
				}
				return exitCode;
			}
			// Project 3 Part 3: M != 1, N != 1
			else{
				if(DEBUG){
					printf("DB: M = %d, N = %d", digM, digN);
				}
				return exitCode;
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
            wrap_file(inText, 1, atoi(argv[2]));
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
