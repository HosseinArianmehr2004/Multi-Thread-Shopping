#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <float.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

int main()
{
    struct dirent *entry;
    DIR *dp = opendir("Dataset/Store1");

    if (dp == NULL)
    {
        perror("Error opening directory");
        return 0;
    }

    pid_t pid = getpid();
    for (int i = 0; i < 8; i++) // create 8 chlderen for each store
    {
        if (pid != 0)
        {
            do
            {
                entry = readdir(dp);

            } while (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0);

            printf("%s\n", entry->d_name);

            pid = fork();
        }
    }

    // printf("%d %d\n", getpid(), pid);
}