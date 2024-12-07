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
float score;
float price_threshold;
char username[100]; // an array of chars is an string :/

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

void *readFile(void *arg)
{
    char *filePath = (char *)arg;

    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        return 0;
    }

    char line[256];

    // Read lines until fgets returns NULL (end of file)
    while (fgets(line, sizeof(line), file) != NULL)
        // printf("%s", line);

        fclose(file);
}

void process_directory(int pid, const char *path)
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

        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_REG) // Check if it's a regular file
        {
            pthread_t thread;
            pthread_create(&thread, NULL, readFile, (void *)strdup(fullPath));
            char file_id[9] = create_file_id(entry->name);
            printf("PID %d create thread for %sID TID: %lu \n", getpid(), entry->d_name, (unsigned long)thread);
            pthread_detach(thread); // Detach the thread to avoid joining
        }
        else if (entry->d_type == DT_DIR)
        {
            if (fork() == 0)
            {
                printf("PID %d create child for %s PID: %d \n", main_pid, entry->d_name, getpid());
                process_directory(getpid(), fullPath); // Recursively process subdirectories
                exit(0);
            }
        }
    }

    while (wait(NULL) != -1 || errno != ECHILD)
        ; // waits until all the children terminate
    closedir(dp);
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
        process_directory(pid, "Dataset/Store1");
    }
    else if (pid1 == 0 && pid2 != 0)
    {
        // int pid = getpid();
        // printf("PID %d create child for Store2 PID: %d \n", main_pid, pid);
        // process_directory(pid, "Dataset/Store2");
    }
    else if (pid1 != 0 && pid2 == 0)
    {
        // int pid = getpid();
        // printf("PID %d create child for Store3 PID: %d \n", main_pid, pid);
        // process_directory(pid, "Dataset/Store3");
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