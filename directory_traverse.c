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

void navDir(char *a, int b)
{
    printf("%d: Navigate this folder: %s\n", b - 1, a);
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
            if (S_ISREG(temp.st_mode))
            {
                printf("Text File: %s\n", de->d_name);
            }
            else if (S_ISDIR(temp.st_mode))
            {
                printf("Directory: %s\n", de->d_name);
                navDir(de->d_name, b);
                chdir("..");
            }
        }
    }
    closedir(dr);
    return;
}

int main(void)
{
    navDir("test_folder", 1);
    return 0;
}