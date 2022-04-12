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

void navDir_v1(char *a, int b, char *filepath)
{
    struct dirent *de;
    chdir(a);
    DIR *dr = opendir(".");

    if (dr == NULL)
    {
        printf("%d: Empty directory: %s\n", b, a);
        return;
    }
    while ((de = readdir(dr)) != NULL)
    {
        struct stat temp;
        if (stat(de->d_name, &temp) != -1 && de->d_name[0] != '.')
        {
            if (S_ISDIR(temp.st_mode))
            {
                // Create filepath
                char *path = filepath;
                char *file = de->d_name;
                int plen = strlen(path);
                int flen = strlen(file);
                char *newpath = malloc(plen + flen + 2);
                memcpy(newpath, path, plen);
                newpath[plen] = '/';
                memcpy(newpath + plen + 1, file, flen + 1);
                printf("%s\n", newpath);

                // Navigate to next folder
                navDir_v1(de->d_name, b, newpath);
                chdir("..");
            }
        }
    }
    closedir(dr);
    return;
}

void navDir_v2(char *a, int b, char *filepath)
{
    struct dirent *de;
    DIR *dr = opendir(filepath);

    if (dr == NULL)
    {
        printf("%d: Empty directory: %s\n", b, a);
        return;
    }
    while ((de = readdir(dr)) != NULL)
    {
        struct stat temp;
        // Create filepath
        char *path = filepath;
        char *file = de->d_name;
        int plen = strlen(path);
        int flen = strlen(file);
        char *newpath = malloc(plen + flen + 2);
        memcpy(newpath, path, plen);
        newpath[plen] = '/';
        memcpy(newpath + plen + 1, file, flen + 1);
        printf("%s\n", newpath);
        if (stat(newpath, &temp) != -1 && de->d_name[0] != '.')
        {
            if (S_ISDIR(temp.st_mode))
            {

                // Navigate to next folder
                printf("%s\n", newpath);
                navDir_v2(de->d_name, b, newpath);
            }
        }
    }
    closedir(dr);
    return;
}

int main(void)
{
    // navDir_v1("test_folder", 1, "./test_folder");
    navDir_v2("test_folder", 1, "./test_folder");
    /*
    struct dirent *de;
    DIR *dr = opendir("test_folder");

    while ((de = readdir(dr)) != NULL)
    {
        struct stat temp;
        if (stat("./test_folder/next_folder", &temp) != -1 && de->d_name[0] != '.')
        {
            if (S_ISREG(temp.st_mode))
            {
                printf("File: %s\n", de->d_name);
            }
            else if (S_ISDIR(temp.st_mode))
            {
                printf("Directory: %s\n", de->d_name);
            }
        }
    }
    */
    return 0;
}