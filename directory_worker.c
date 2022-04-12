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

void navDir(char *a, int b){
    printf("%d: Navigate this folder: %s\n", b-1, a);
    struct dirent *de;
    DIR *dr = opendir(".");

    if(dr == NULL){
        printf("%d: Empty directory: %s\n", b, a);
        return;
    }
    while ((de = readdir(dr)) != NULL){
        struct stat temp;
        if(stat(de->d_name, &temp) != -1 && de->d_name[0] != '.'){
            if(S_ISREG(temp.st_mode)){
                printf("%d: Wrap this file: %s : %s\n", b, a, de->d_name);
                //add file path to file work queue
            }
            else if(S_ISDIR(temp.st_mode)){
                char cwd[100];
                //char *cwd = calloc(100, sizeof(char));
                if(getcwd(cwd, sizeof(cwd)) != NULL){
                    printf("Currently working dir: %s\n", cwd);
                    chdir(de->d_name);
                    if(getcwd(cwd, sizeof(cwd)) != NULL){
                        printf("Moving to dir: %s\n", cwd);
                        navDir(de->d_name, b++);
                    }
                    chdir("..");
                }
                //free(cwd);
            }
        }
    }
    closedir(dr);
    return;
}

int main(void){
    struct dirent *de;
    DIR *dr = opendir(".");
  
    if (dr == NULL){
        printf("Null directory" );
        return 0;
    }
    while ((de = readdir(dr)) != NULL){
        struct stat temp;
        if(stat(de->d_name, &temp) != -1 && de->d_name[0] != '.'){
            if(S_ISREG(temp.st_mode)){
                printf("Wrap this file: %s\n", de->d_name);
            }
            else if(S_ISDIR(temp.st_mode)){
                char cwd[100];
                //char *cwd = calloc(100, sizeof(char));
                if(getcwd(cwd, sizeof(cwd)) != NULL){
                    printf("Currently working dir: %s\n", cwd);
                    chdir(de->d_name);
                    if(getcwd(cwd, sizeof(cwd)) != NULL){
                        printf("Moving to dir: %s\n", cwd);
                        navDir(de->d_name, 1);
                    }
                    chdir("..");
                }
                //free(cwd);
            }
        }
    }

    closedir(dr);    
    return 0;
}