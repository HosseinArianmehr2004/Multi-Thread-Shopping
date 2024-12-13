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

typedef struct
{
    char name[50];
    int number;
} order_list;

typedef struct
{
    char path[200];
    char store_number;

    int number;

    char name[50];
    float price;
    float score;
    int entity;
} item;

typedef struct
{
    item cart[300];
    int count;
} shop_cart;

pid_t main_pid;
pthread_mutex_t mutex;

int results_count = 0;
float price_threshold;
char username[100];
char chosen_store[2];

item results[100];
order_list order_list_items[100];

void login()
{
    // Get username
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;

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
        // Get input (name & number of order list items)
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0) // Input is empty
        {
            strcpy(order_list_items[i].name, "\n");
            break;
        }
        else
        {
            char *last_space = strrchr(input, ' ');
            if (last_space != NULL)
            {
                *last_space = '\0';
                sscanf(last_space + 1, "%d", &order_list_items[i].number);                  // Extract number
                strncpy(order_list_items[i].name, input, sizeof(order_list_items[i].name)); // Extract name
            }
        }
        printf("name: %s, number: %d\n", order_list_items[i].name, order_list_items[i].number);
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

bool is_in_order_list(item *checking_item)
{
    // Search name in the order list
    int i = 0;
    while (strcmp(order_list_items[i].name, "\n") != 0)
    {
        if (strcmp(order_list_items[i].name, checking_item->name) == 0)
        {
            checking_item->number = order_list_items[i].number;
            return true;
        }
        i++;
    }

    return false;
}

void *read_file(void *arg)
{
    char *file_path = (char *)arg;
    item *local_item = malloc(sizeof(item));
    char line[256];

    if (local_item == NULL)
    {
        perror("Memory allocation failed");
        free(file_path);
        return NULL;
    }

    FILE *file = fopen(file_path, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        free(file_path);
        free(local_item);
        return NULL;
    }

    if (fgets(line, sizeof(line), file) != NULL)
        sscanf(line, "Name: %199[^\n]", local_item->name);

    if (is_in_order_list(local_item))
    {
        if (fgets(line, sizeof(line), file) != NULL)
            sscanf(line, "Price: %f", &local_item->price);

        if (fgets(line, sizeof(line), file) != NULL)
            sscanf(line, "Score: %f", &local_item->score);

        if (fgets(line, sizeof(line), file) != NULL)
            sscanf(line, "Entity: %d", &local_item->entity);

        sscanf(file_path, "Dataset/Store%c/", &local_item->store_number);
        strcpy(local_item->path, file_path);

        pthread_mutex_lock(&mutex);
        results[results_count] = *local_item;
        results_count++;
        pthread_mutex_unlock(&mutex);
    }

    // Read lines until fgets returns NULL (end of file)
    // while (fgets(line, sizeof(line), file) != NULL)
    // {
    //     printf("%s", line);
    // }

    fclose(file);
    free(file_path);
    free(local_item);
    return NULL;
}

void create_thread(const char *path)
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

    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(threads[i], NULL);
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
                // printf("PID %d create child for %s PID: %d \n", getppid(), entry->d_name, getpid());

                char fullPath[1024];
                if (snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name) >= 1024)
                {
                    fprintf(stderr, "Path too long: %s/%s\n", path, entry->d_name);
                    exit(EXIT_FAILURE);
                }

                pthread_mutex_init(&mutex, NULL);
                create_thread(fullPath);
                pthread_mutex_destroy(&mutex);

                close(pipe_fd[0]); // Close the read end
                for (int i = 0; i < results_count; i++)
                    if (results[i].name[0] != '\0')
                        write(pipe_fd[1], &results[i], sizeof(item)); // Write the entire struct
                close(pipe_fd[1]);                                    // Close the write end

                closedir(dp);
                exit(0);
            }
        }
    }

    close(pipe_fd[1]); // Close the write end of the pipe in the parent

    *shopping_cart_count = 0;
    float price = 0.0;
    int temp_number = 0;
    int temp_entity = 0;
    item buffer;
    while (read(pipe_fd[0], &buffer, sizeof(item)) > 0)
    {
        temp_number = 0;
        temp_entity = buffer.entity;
        for (int i = 0; i < buffer.number; i++)
        {
            if (temp_number > temp_entity || temp_entity == 0)
            {
                break;
            }
            price += buffer.price;
            if (price > price_threshold)
            {
                price -= buffer.price;
                break;
            }
            temp_entity--; // in bayad tooye filesh ham doros beshe update beshe
            temp_number++;
        }
        buffer.number = temp_number;
        shopping_cart[*shopping_cart_count] = buffer;
        (*shopping_cart_count)++;
    }
    close(pipe_fd[0]); // Close the read end of the pipe

    while (wait(NULL) != -1 || errno != ECHILD)
        ; // Wait for all child processes to terminate

    closedir(dp);
}

void *order(void *arg)
{
    shop_cart *data = (shop_cart *)arg;
    item *cart = data->cart; // the shopping cart
    int count = data->count;

    pthread_t thread_id = pthread_self();
    printf("PID %d create thread for Orders TID: %lu \n", getpid(), (unsigned long)thread_id);

    // Calculate the value of the shopping cart
    float shopping_cart_value[3] = {0.0, 0.0, 0.0};
    for (int i = 0; i < count; i++)
    {
        if (cart[i].store_number == '1')
        {
            shopping_cart_value[0] += cart[i].score * cart[i].number * 10 / cart[i].price;
        }
        if (cart[i].store_number == '2')
        {
            shopping_cart_value[1] += cart[i].score * cart[i].number * 10 / cart[i].price;
        }
        if (cart[i].store_number == '3')
        {
            shopping_cart_value[2] += cart[i].score * cart[i].number * 10 / cart[i].price;
        }
    }
    printf("Shopping cart value 1 : %.3f\n", shopping_cart_value[0]);
    printf("Shopping cart value 2 : %.3f\n", shopping_cart_value[1]);
    printf("Shopping cart value 3 : %.3f\n", shopping_cart_value[2]);

    // Find max shopping cart value
    char best_store[50];
    float max = 0;
    for (int i = 0; i < 3; i++)
    {
        if (shopping_cart_value[i] > max)
        {
            max = shopping_cart_value[i];
            sprintf(best_store, "Max shopping cart value is %.3f for Store %d !\n", max, i + 1);
            sprintf(chosen_store, "%d", i + 1);
        }
    }
    printf("%s", best_store);
    return NULL;
}

void *score(void *arg)
{
    shop_cart *data = (shop_cart *)arg;
    item *cart = data->cart; // the shopping cart
    int count = data->count;

    float item_scores[100];
    int item_scores_count = 0;

    pthread_t thread_id = pthread_self(); // Get the thread ID
    printf("PID %d create thread for Scores TID: %lu \n", main_pid, (unsigned long)thread_id);

    // Get score
    printf("Chosen store: %s \n", chosen_store);
    printf("Enter score in range [0, 5] for:\n");
    for (int i = 0; i < count; i++)
    {
        if (cart[i].store_number != chosen_store[0])
        {
            continue;
        }

        printf("%s: ", cart[i].name);
        char input[50];
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
        sscanf(input, "%f", &item_scores[item_scores_count]);

        if (item_scores[item_scores_count] < 0.0 || item_scores[item_scores_count] > 5.0)
        {
            printf("435\n");
            printf("The score entered is not valid !\n");
            i--;
            continue;
        }

        item_scores_count++;
    }

    for (int i = 0; i < item_scores_count; i++)
    {
        printf("%.2f\n", item_scores[i]);
    }
    return NULL;
}

void * final(void *arg)
{
    pthread_t thread_id = pthread_self(); // Get the thread ID
    // printf("PID %d create thread for Final TID: %lu \n", main_pid, (unsigned long)thread_id);

    return NULL;
}

int main()
{
    login();
    get_order_list();

    main_pid = getpid();
    printf("%s create PID: %d\n", username, main_pid);

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1)
    {
        perror("Pipe failed");
        return 1;
    }

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

    char store_path[20];
    char store_number;

    if (pid1 == 0 && pid2 != 0)
    {
        strcpy(store_path, "Dataset/Store1");
        store_number = '1';
    }
    else if (pid1 == 0 && pid2 == 0)
    {
        strcpy(store_path, "Dataset/Store2");
        store_number = '2';
    }
    else if (pid1 != 0 && pid2 == 0)
    {
        strcpy(store_path, "Dataset/Store3");
        store_number = '3';
    }

    if (pid1 == 0 || pid2 == 0) // All children processes
    {
        item shopping_cart[100];
        int shopping_cart_count = 0;
        float cart_price = 0.0;

        printf("PID %d create child for Store%c PID: %d \n", main_pid, store_number, getpid());
        create_process(store_path, shopping_cart, &shopping_cart_count);

        // Send shopping cart data to parent via pipe
        close(pipe_fd[0]); // Close read end
        write(pipe_fd[1], shopping_cart, sizeof(item) * shopping_cart_count);
        close(pipe_fd[1]); // Close write end

        exit(0);
    }
    else // Parent process
    {
        while (wait(NULL) != -1 || errno != ECHILD)
            ; // waits until all the children terminate

        shop_cart all;
        all.count = 0;

        close(pipe_fd[1]); // Close write end
        while (read(pipe_fd[0], &all.cart[all.count], sizeof(item)) > 0)
        {
            all.count++;
        }
        close(pipe_fd[0]); // Close read end

        for (int i = 0; i < all.count; i++)
        {
            printf("Store %c : %s: Price = %.2f, Score = %.1f, Entity = %d, Number = %d\n",
                   all.cart[i].store_number, all.cart[i].name, all.cart[i].price,
                   all.cart[i].score, all.cart[i].entity, all.cart[i].number);
        }

        pthread_t orders_th, scores_th, final_th;
        pthread_create(&orders_th, NULL, order, (void *)&all);
        pthread_create(&scores_th, NULL, score, (void *)&all);
        pthread_create(&final_th, NULL, final, NULL);

        pthread_join(orders_th, NULL);
        pthread_join(scores_th, NULL);
        pthread_join(final_th, NULL);
    }

    return 0;
}