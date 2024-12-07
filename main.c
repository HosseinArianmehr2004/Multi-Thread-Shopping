#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

int main_pid;
char username[100]; // an array of chars is an string :/
float score;
float price_threshold;

typedef struct
{
    char name[50];
    int number;
} order_list;

void login()
{
    // Get username
    printf("Enter your username: ");
    scanf("%99s", username);

    // Create user file
    char file_path[200];
    snprintf(file_path, sizeof(file_path), "Users/%s.txt", username);
    FILE *file = fopen(file_path, "w");
    if (file == NULL)
    {
        perror("Error creating file");
        // return EXIT_FAILURE;
    }

    // Writing user information
    fprintf(file, "Username: %s\n", username);
    fprintf(file, "Number of times purchased from the Store1: %d\n", 0);
    fprintf(file, "Number of times purchased from the Store3: %d\n", 0);
    fprintf(file, "Number of times purchased from the Store2: %d\n", 0);
    fclose(file);
}

void get_order_list()
{
    order_list item[100];
    printf("Orderlist0:\n");
    int i = 0;
    while (1)
    {
        scanf("%49s %d", item[i].name, &item[i].number);
        // if (strcmp(item[i].name, "\n") == 0)
        // {
        //     break;
        // }
        printf("name: %s, Number: %d\n", item[i].name, item[i].number);
        i++;
    }

    // Get price threshold
    printf("Enter your price threshold: ");
    scanf("%f", &price_threshold);
}

void *read_file(void *arg)
{
    char *filePath = (char *)arg;
    char *name = malloc(200);
    char line[256];

    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        free(filePath);
        return NULL; // Return NULL on error
    }

    // Read lines until we find the name
    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "Name: %199[^\n]", name) == 1)
        {
            break; // Successfully read the name
        }
    }

    // search name in the order list

    // Read lines until fgets returns NULL (end of file)
    // while (fgets(line, sizeof(line), file) != NULL)
    // {
    //     printf("%s", line);
    // }

    fclose(file);
    free(filePath);

    return (void *)name;
}

void create_thread(const char *path) // all threads created by a process
{
    // this function do all the stuff that a process (the process which creates thread for files)

    struct dirent *entry;
    DIR *dp = opendir(path);

    if (dp == NULL)
    {
        perror("Error opening directory");
        return;
    }

    pthread_t threads[100];
    int thread_count = 0;
    char *results[100]; // array of strings contains the threads return values

    while ((entry = readdir(dp)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

        // Check if it's a regular file
        if (entry->d_type == DT_REG)
        {
            if (pthread_create(&threads[thread_count], NULL, read_file, (void *)strdup(fullPath)) != 0)
            {
                perror("Failed to create thread");
                closedir(dp);
                return;
            }

            // printf("PID %d create thread for %sID TID: %lu \n", getpid(), entry->d_name, (unsigned long)threads[thread_count]);
            thread_count++;
        }
    }

    for (int i = 0; i < thread_count; i++)
    {
        void *result;
        pthread_join(threads[i], &result); // Correctly retrieve the result
        if (result != NULL)
        {
            results[i] = (char *)result;
            printf("PID %d Thread returned: %s\n", getpid(), results[i]);
            free(result); // Free the memory allocated in read_file function
        }
    }

    // do all the stuff that a process has to do
    // there is no need to return anything to create_process function

    closedir(dp);
}

void create_process(const char *path)
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

                // printf("PID %d create child for %s PID: %d \n", getppid(), entry->d_name, getpid());

                create_thread(fullPath);
                exit(0);
            }
        }
    }

    closedir(dp);

    while (wait(NULL) != -1 || errno != ECHILD)
        ; // waits until all the children terminate
}

void get_score()
{
    printf("Enter your score for this purchase: ");
    scanf("%f", &score);
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
        // create_process(pid, "Dataset/Store1");
        create_process("temp_dataset/Store1");
    }
    else if (pid1 == 0 && pid2 != 0)
    {
        // int pid = getpid();
        // printf("PID %d create child for Store2 PID: %d \n", main_pid, pid);
        // create_process("Dataset/Store2");
    }
    else if (pid1 != 0 && pid2 == 0)
    {
        // int pid = getpid();
        // printf("PID %d create child for Store3 PID: %d \n", main_pid, pid);
        // create_process("Dataset/Store3");
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

    // get_score();

    return 0;
}