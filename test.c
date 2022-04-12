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

int main()
{
    struct dirent *de;
    DIR *dr = opendir("./");

    if (dr == NULL)
    {
        printf("Null directory");
        return 0;
    }


}