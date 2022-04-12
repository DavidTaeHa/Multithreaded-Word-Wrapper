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

void navDir(char *a, int b, char *filepath)
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
                //Create filepath
                char *path = filepath;
                char *file = de->d_name;
                int plen = strlen(path);
                int flen = strlen(file);
                char *newpath = malloc(plen + flen + 2);
                memcpy(newpath, path, plen);
                newpath[plen] = '/';
                memcpy(newpath + plen + 1, file, flen + 1);
                printf("%s\n", newpath);

                //Navigate to next folder
                navDir(de->d_name, b, newpath);
                chdir("..");
            }
        }
    }
    closedir(dr);
    return;
}

int main(void)
{
    navDir("test_folder", 1, "./test_folder");
    return 0;
}