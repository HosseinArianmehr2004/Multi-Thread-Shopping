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

int main_pid;
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

void *read_file(void *arg)
{
    char *filePath = (char *)arg;
    char *name = malloc(200); // Allocate memory for name
    char line[256];

    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        free(filePath); // Free the filePath memory
        return NULL;    // Return NULL on error
    }

    // Read lines until we find the name
    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "Name: %199[^\n]", name) == 1)
        {
            break; // Successfully read the name
        }
    }

    fclose(file);
    free(filePath);      // Free the filePath memory
    return (void *)name; // Return the allocated name
}

void create_thread(int pid, const char *path)
{
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (dp == NULL)
    {
        perror("Error opening directory");
        return;
    }

    pthread_t threads[100]; // Assume a maximum of 100 files
    char *results[100];     // Array to hold results
    int thread_count = 0;

    while ((entry = readdir(dp)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

        // Check if it's a regular file
        if (entry->d_type == DT_REG)
        {
            char *arg = strdup(fullPath); // Duplicate the fullPath
            if (pthread_create(&threads[thread_count], NULL, read_file, (void *)arg) != 0)
            {
                perror("Failed to create thread");
                free(arg); // Free the duplicated path on error
                closedir(dp);
                return;
            }
            thread_count++;
        }
    }

    for (int i = 0; i < thread_count; i++)
    {
        void *result;
        pthread_join(threads[i], &result); // Correctly retrieve the result
        if (result != NULL)
        {
            printf("Thread returned: %s\n", (char *)result);
            free(result); // Free the memory allocated in read_file
        }
    }

    closedir(dp);
}
void create_process(int pid, const char *path)
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
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        if (entry->d_type == DT_DIR)
        {
            if (fork() == 0)
            {
                char fullPath[1024];
                snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

                // printf("PID %d create child for %s PID: %d \n", pid, entry->d_name, getpid());

                create_thread(pid, fullPath);
                exit(0);
            }
        }
    }

    closedir(dp);

    while (wait(NULL) != -1 || errno != ECHILD)
        ; // waits until all the children terminate
}

void *order(void *arg)
{
    pthread_t thread_id = pthread_self(); // Get the thread ID
    printf("PID %d create thread for Orders TID: %lu \n", main_pid, (unsigned long)thread_id);
}

void *score(void *arg)
{
    pthread_t thread_id = pthread_self(); // Get the thread ID
    printf("PID %d create thread for Scores TID: %lu \n", main_pid, (unsigned long)thread_id);
}

void * final(void *arg)
{
    pthread_t thread_id = pthread_self(); // Get the thread ID
    printf("PID %d create thread for Final TID: %lu \n", main_pid, (unsigned long)thread_id);
}

int main()
{
    // login();
    // get_order_list();

    main_pid = getpid(); // the main process id
    printf("%s create PID: %d\n", username, main_pid);

    int pid1 = fork();
    int pid2 = fork();

    if (pid1 == 0 && pid2 == 0)
    {
        int pid = getpid();
        printf("PID %d create child for Store1 PID: %d \n", main_pid, pid);
        create_process(pid, "../Dataset/Store1");
    }
    else if (pid1 == 0 && pid2 != 0)
    {
        // int pid = getpid();
        // printf("PID %d create child for Store2 PID: %d \n", main_pid, pid);
        // create_process(pid, "Dataset/Store2");
    }
    else if (pid1 != 0 && pid2 == 0)
    {
        // int pid = getpid();
        // printf("PID %d create child for Store3 PID: %d \n", main_pid, pid);
        // create_process(pid, "Dataset/Store3");
    }
    else
    {
        // Parent process

        // pthread_t orders_th, scores_th, final_th;
        // pthread_create(&orders_th, NULL, order, NULL);
        // pthread_create(&scores_th, NULL, score, NULL);
        // pthread_create(&final_th, NULL, final, NULL);

        while (wait(NULL) != -1 || errno != ECHILD)
            ; // waits until all the children terminate
    }

    return 0;
}
