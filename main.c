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

int main_pid;
char username[100]; // an array of chars is an string :/
float price_threshold;

typedef struct
{
    char name[200];
    int number;
} order_list;

typedef struct
{
    char path[200];

    char name[200];
    float price;
    float score;
    int entity;
} item;

order_list order_list_items[100];

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
    char input[100];
    while (1)
    {
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0) // Input is empty
        {
            strcpy(order_list_items[i].name, "\n");
            break;
        }
        else
        {
            sscanf(input, "%49s %d", order_list_items[i].name, &order_list_items[i].number);
        }
        printf("name: %s, Number: %d\n", order_list_items[i].name, order_list_items[i].number);
        i++;
    }

    // Get price threshold
    printf("Enter your price threshold: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;
    if (strlen(input) == 0)
    {
        price_threshold = FLT_MAX; // Input is empty
    }
    else
    {
        sscanf(input, "%f", &price_threshold);
    }
    printf("\n");
}

bool is_in_order_list(char *name)
{
    // Search name in the order list
    int i = 0;
    while (strcmp(order_list_items[i].name, "\n") != 0)
    {
        if (strcmp(order_list_items[i].name, name) == 0)
            return true;

        i++;
    }

    return false;
}

void *read_file(void *arg)
{
    char *filePath = (char *)arg;
    char line[256];
    item *local_item = malloc(sizeof(item));

    if (local_item == NULL)
    {
        perror("Memory allocation failed");
        free(filePath);
        return NULL;
    }

    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        free(filePath);
        free(local_item);
        return NULL;
    }

    if (fgets(line, sizeof(line), file) != NULL)
        sscanf(line, "Name: %199[^\n]", local_item->name);

    if (is_in_order_list(local_item->name))
    {
        if (fgets(line, sizeof(line), file) != NULL)
            sscanf(line, "Price: %f", &local_item->price);

        if (fgets(line, sizeof(line), file) != NULL)
            sscanf(line, "Score: %f", &local_item->score);

        if (fgets(line, sizeof(line), file) != NULL)
            sscanf(line, "Entity: %d", &local_item->entity);

        strcpy(local_item->path, filePath);
        fclose(file);
        free(filePath);
        return (void *)local_item;
    }

    // Read lines until fgets returns NULL (end of file)
    // while (fgets(line, sizeof(line), file) != NULL)
    // {
    //     printf("%s", line);
    // }

    fclose(file);
    free(filePath);
    free(local_item);
    return NULL;
}

void create_thread(const char *path, item results[], int *results_count)
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
            char str[50];
            char product_ID[50];
            strncpy(str, entry->d_name, strlen(entry->d_name) - 4);
            if (strlen(str) < 6)
            {
                snprintf(product_ID, sizeof(product_ID), "%0*d%s", 6 - strlen(str), 0, str);
            }
            else
            {
                strcpy(product_ID, str);
            }
            // printf("PID %d create thread for %sID TID: %lu \n", getpid(), product_ID, (unsigned long)threads[thread_count]);
            thread_count++;
        }
    }

    void *result;
    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(threads[i], &result);
        if (result != NULL)
        {
            results[*results_count] = *(item *)result; // copy the struct
            (*results_count)++;
            // printf("PID %d Thread returned: %s \n", getpid(), results[*results_count - 1]);
            free(result); // Free the memory allocated in read_file
        }
    }

    closedir(dp);
}

void create_process(const char *path, item shopping_cart[], int *shopping_cart_count)
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

                item results[100];
                int results_count = 0;

                create_thread(fullPath, results, &results_count);

                for (int i = 0; i < results_count; i++)
                    if (results[i].name[0] != '\0')
                        write(pipe_fd[1], &results[i], sizeof(item)); // Write the entire struct

                close(pipe_fd[1]); // Close the write end after writing
                closedir(dp);
                exit(0);
            }
        }
    }

    close(pipe_fd[1]); // Close the write end of the pipe in the parent

    *shopping_cart_count = 0;
    float price = 0.0;
    item buffer;
    while (read(pipe_fd[0], &buffer, sizeof(item)) > 0)
    {
        price += buffer.price;
        if (price > price_threshold)
        {
            price -= buffer.price;
            continue;
        }
        else
        {
            shopping_cart[*shopping_cart_count] = buffer;
            (*shopping_cart_count)++;
            // printf("Parent received: %s, Price: %.2f, Score: %.2f, Entity: %d\n", buffer.name, buffer.price, buffer.score, buffer.entity);
        }
    }

    close(pipe_fd[0]); // Close the read end of the pipe
    closedir(dp);

    while (wait(NULL) != -1 || errno != ECHILD)
        ; // Wait for all child processes to terminate
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

pthread_mutex_t print_mutex; // Mutex for controlling print access

int main()
{
    pthread_mutex_init(&print_mutex, NULL); // Initialize the mutex

    // age login ro az comment dar biarim kharab mishe
    // login();
    get_order_list();

    pid_t main_pid = getpid();
    printf("%s create PID: %d\n", username, main_pid);

    pid_t pid1 = fork();
    if (pid1 < 0)
    {
        perror("Fork failed");
        return 1;
    }

    pid_t pid2 = fork();
    if (pid2 < 0)
    {
        perror("Fork failed");
        return 1;
    }

    char store_path[200];
    char store[10];

    if (pid1 == 0 && pid2 == 0)
    {
        strcpy(store_path, "Dataset/Store1");
        strcpy(store, "Store 1");
    }
    else if (pid1 == 0 && pid2 != 0)
    {
        strcpy(store_path, "Dataset/Store2");
        strcpy(store, "Store 2");
    }
    else if (pid1 != 0 && pid2 == 0)
    {
        strcpy(store_path, "Dataset/Store3");
        strcpy(store, "Store 3");
    }

    if (pid1 == 0 || pid2 == 0) // All children processes
    {
        item shopping_cart[100];
        int shopping_cart_count = 0;

        printf("PID %d create child for %s PID: %d \n", main_pid, store, getpid());
        create_process(store_path, shopping_cart, &shopping_cart_count);

        // useing mutexes maybe works incorrect
        pthread_mutex_lock(&print_mutex); // Lock the mutex before printing
        printf("\n%s shop cart: \n", store);
        for (int i = 0; i < shopping_cart_count; i++)
        {
            printf("%s: Price = %f, Score = %.2f, Entity = %d\n", shopping_cart[i].name, shopping_cart[i].price, shopping_cart[i].score, shopping_cart[i].entity);
        }
        pthread_mutex_unlock(&print_mutex); // Unlock the mutex after printing

        exit(0);
    }
    else // Parent process
    {
        // pthread_t orders_th, scores_th, final_th;
        // pthread_create(&orders_th, NULL, order, NULL);
        // pthread_create(&scores_th, NULL, score, NULL);
        // pthread_create(&final_th, NULL, final, NULL);

        while (wait(NULL) != -1 || errno != ECHILD)
            ; // waits until all the children terminate
    }

    pthread_mutex_destroy(&print_mutex); // Destroy the mutex
    return 0;
}