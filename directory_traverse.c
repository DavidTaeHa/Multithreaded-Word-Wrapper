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

//This version of the method traverse recursively through all of the subdirectories
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

//This version only traverses through current given directory
void navDir(char *a)
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
            }
        }
    }
    closedir(dr);
    return;
}

void *directory_worker(void *args){

}

int main()
{
    //navDir_whole("./test_folder");
    //navDir("./test_folder");
    //navDir("./test_folder/next_folder");
    return 0;
}