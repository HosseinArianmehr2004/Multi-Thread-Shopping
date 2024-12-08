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
float price_threshold;

typedef struct
{
    char name[200];
    int number;
} order_list;

order_list item[100];

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
    printf("Orderlist0:\n");
    int i = 0;
    while (1)
    {
        scanf("%49s %d", item[i].name, &item[i].number);
        if (strcmp(item[i].name, "done") == 0)
        {
            break;
        }
        printf("name: %s, Number: %d\n", item[i].name, item[i].number);
        i++;
    }

    // Get price threshold
    printf("Enter your price threshold: ");
    scanf("%f", &price_threshold);
    printf("\n");
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
        return NULL;
    }

    // Read lines until we find the name
    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "Name: %199[^\n]", name) == 1)
        {
            break; // Successfully read the name
        }
    }

    // Search name in the order list
    int i = 0;
    while (strcmp(item[i].name, "done") != 0)
    {
        if (strcmp(item[i].name, name) == 0)
        {
            // printf("100\n");
            fclose(file);
            free(filePath);
            return (void *)name; // Successfully find the name
        }
        i++;
    }

    // Read lines until fgets returns NULL (end of file)
    // while (fgets(line, sizeof(line), file) != NULL)
    // {
    //     printf("%s", line);
    // }

    // The name not found
    fclose(file);
    free(filePath);
    return NULL;
}

void create_thread(const char *path, char *results[], int *results_count)
{
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (dp == NULL)
    {
        perror("Error opening directory");
        return;
    }

    pthread_t threads[100];
    int thread_count = 0;
    *results_count = 0;

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

            // thread id for line below
            // printf("PID %d create thread for %sID TID: %lu \n", getpid(), entry->d_name, (unsigned long)threads[thread_count]);
            thread_count++;
        }
    }

    void *result;
    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(threads[i], &result);
        if (result != NULL)
        {
            results[*results_count] = (char *)result;
            (*results_count)++;
            // printf("PID %d Thread returned: %s \n", getpid(), results[*results_count - 1]);
        }
    }

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

    int pipe_fd[2]; // Pipe file descriptors

    if (pipe(pipe_fd) == -1) // Create a pipe
    {
        perror("Pipe failed");
        closedir(dp);
        return;
    }

    while ((entry = readdir(dp)) != NULL)
    {
        // Skip current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // Check if the entry is a directory
        if (entry->d_type == DT_DIR)
        {
            pid_t pid = fork();
            if (pid < 0) // Error in fork
            {
                perror("Fork failed");
                closedir(dp);
                return;
            }
            if (pid == 0) // Child process
            {
                close(pipe_fd[0]); // Close the read end of the pipe

                char fullPath[1024];
                if (snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name) >= 1024)
                {
                    fprintf(stderr, "Path too long: %s/%s\n", path, entry->d_name);
                    exit(EXIT_FAILURE);
                }

                // printf("PID %d create child for %s PID: %d \n", getppid(), entry->d_name, getpid());

                char *results[100]; // array of strings contains the threads return values
                int results_count;

                create_thread(fullPath, results, &results_count);

                for (int i = 0; i < results_count; i++)
                {
                    if (results[i] != NULL)
                    {
                        size_t length = strlen(results[i]);
                        write(pipe_fd[1], &length, sizeof(size_t)); // Write the length first
                        write(pipe_fd[1], results[i], length + 1);  // Write the string (including null terminator)
                        free(results[i]);                           // Free the memory allocated in read_file function
                    }
                }

                close(pipe_fd[1]); // Close the write end after writing
                closedir(dp);

                exit(0);
            }
        }
    }

    close(pipe_fd[1]); // Close the write end of the pipe in the parent

    char *shopping_cart[100];
    int shopping_cart_count = 0;

    size_t length;
    while (read(pipe_fd[0], &length, sizeof(size_t)) > 0)
    {
        char *buffer = malloc(length + 1); // +1 for null terminator
        if (buffer == NULL)
        {
            perror("Memory allocation failed");
            break;
        }
        read(pipe_fd[0], buffer, length + 1); // Read the string including null terminator

        shopping_cart[shopping_cart_count] = buffer;
        shopping_cart_count++;

        printf("Parent received: %s\n", buffer);
        // the buffers deallocated in the for loop below
    }

    close(pipe_fd[0]); // Close the read end of the pipe
    closedir(dp);

    for (int i; i < shopping_cart_count; i++)
        free(shopping_cart[shopping_cart_count]); // Free the allocated memory

    // Wait for all child processes to terminate
    while (wait(NULL) != -1 || errno != ECHILD)
        ;
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
    get_order_list();

    main_pid = getpid(); // the main process id
    printf("%s create PID: %d\n", username, main_pid);

    int pid1 = fork();
    int pid2 = fork();

    if (pid1 == 0 && pid2 == 0)
    {
        int pid = getpid();
        printf("PID %d create child for Store1 PID: %d \n", main_pid, pid);
        create_process("Dataset/Store1");
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

    return 0;
}