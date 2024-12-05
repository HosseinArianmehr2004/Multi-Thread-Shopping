#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

char username[100];       // an array of chars is an string :/
char order_list[100][50]; // Array with 100 rows of strings, each with length 50

void login()
{
    printf("Username: ");
    scanf("%99s", username);

    char file_path[200];
    snprintf(file_path, sizeof(file_path), "Users/%s.txt", username);

    FILE *file;
    file = fopen(file_path, "w");
    if (file == NULL)
    {
        perror("Error creating file");
        // return EXIT_FAILURE;
    }
    fprintf(file, "Username: %s\n", username);
    fprintf(file, "Purchase Times: %d", 0);
    fclose(file);
}

void get_order_list()
{
    printf("Orderlist0:\n");

    char input[50];
    for (int i = 0; i < 100; i++)
    {
        scanf("%49s", input);

        if (strcmp(input, "done") == 0)
            break;

        strcpy(order_list[i], input);
    }

    int threshold;
    printf("Price threshold: ");
    scanf("%d", &threshold);
}

int countFilesInDirectory(const char *path)
{
    struct dirent *entry;
    DIR *dp = opendir(path);
    int fileCount = 0;

    if (dp == NULL)
    {
        perror("Error opening directory");
        return 0;
    }

    while ((entry = readdir(dp)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

        // Use stat to check if the entry is a directory
        struct stat statbuf;
        if (stat(fullPath, &statbuf) == 0)
        {
            if (S_ISDIR(statbuf.st_mode))
                fileCount += countFilesInDirectory(fullPath);
            else
                fileCount++;
        }
    }

    closedir(dp);
    return fileCount;
}

void *readFile(const char *filePath)
{
    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return 0;
    }

    char line[256];

    // Read lines until fgets returns NULL (end of file)
    while (fgets(line, sizeof(line), file) != NULL)
        printf("%s", line);

    fclose(file);
}

void processDirectory(const char *path)
{
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (dp == NULL)
    {
        perror("Error opening directory");
        return;
    }

    while ((entry = readdir(dp)) != NULL)
    {
        // Skip the current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

        // Check if the entry is a directory
        if (entry->d_type == DT_DIR)
        {
            pid_t pid = fork();
            if (pid == 0)
            {
                int num_files = countFilesInDirectory(fullPath);
                pthread_t thread[num_files];

                for (int i = 0; i < num_files; i++)
                {
                    pthread_create(&thread[i], NULL, (void *)readFile, fullPath);
                    // printf("Thread %d has started\n", i);
                }

                for (int i = 0; i < num_files; i++)
                {
                    pthread_join(thread[i], NULL);
                    // printf("Thread %d has finished execution\n", i);
                }

                // printf("numfiles in %s = %d\n", fullPath, num_files);
                exit(0);
            }
            else if (pid > 0)
            {
                wait(NULL);
            }
            else
            {
                perror("Fork failed");
            }
        }
    }

    closedir(dp);
}

int main()
{
    // login();
    // get_order_list();

    int pid1 = fork();
    int pid2 = fork();

    if (pid1 == 0 && pid2 == 0)
    {
        processDirectory("Dataset/Store1");
    }
    else if (pid1 == 0 && pid2 != 0)
    {
        processDirectory("Dataset/Store2");
    }
    else if (pid1 != 0 && pid2 == 0)
    {
        processDirectory("Dataset/Store3");
    }
    else
    {
        // Parent process

        while (wait(NULL) != -1 || errno != ECHILD)
            ; // waits until all the children terminate
    }

    return 0;
}
