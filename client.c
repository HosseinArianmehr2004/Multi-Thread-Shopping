#include <time.h>
#include <stdio.h>
#include <float.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
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
    char name[50];
    float price;
    float score;
    int entity;
    char last_modified[100];

    int number;
    char store_number;
    char path[200];

    pthread_t thread_id;
    pid_t process_id;
} item;

typedef struct
{
    item cart[300];
    int count;
} shop_cart;

pid_t main_pid;
bool get_score_lock = true;

char username[100];
int order_number;
float price_threshold = FLT_MAX;
char chosen_store[2];
order_list order_list_items[100];

char log_file_path[1024];
pthread_mutex_t log_file_mutex;

item results[100];
int results_count = 0;
pthread_mutex_t results_array_mutex; // to put the items in the results correctly

void get_order_list()
{
    printf("Orderlist%d:\n", order_number);
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
        // printf("name: %s, number: %d\n", order_list_items[i].name, order_list_items[i].number);
        i++;
    }

    // Get price threshold
    printf("Enter your price threshold: ");
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;
    if (strlen(input) != 0)
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

    local_item->process_id = getpid();
    local_item->thread_id = pthread_self();

    if (fgets(line, sizeof(line), file) != NULL)
    {
        sscanf(line, "Name: %49[^\n]", local_item->name);
    }

    // Write in log file
    pthread_mutex_lock(&log_file_mutex);
    FILE *log_file = fopen(log_file_path, "a");
    if (log_file == NULL)
    {
        perror("Error opening file");
        fclose(file);
        free(file_path);
        free(local_item);
        return NULL;
    }
    if (is_in_order_list(local_item))
    {
        fprintf(log_file, "Thread with TID = %lu       (read and found): ((%s))\n", local_item->thread_id, local_item->name);
    }
    else
    {
        fprintf(log_file, "Thread with TID = %lu read: %s\n", local_item->thread_id, local_item->name);
    }
    fclose(log_file);
    pthread_mutex_unlock(&log_file_mutex);

    // Read all the founded items file content
    if (is_in_order_list(local_item))
    {
        while (fgets(line, sizeof(line), file))
        {
            line[strcspn(line, "\n")] = 0;

            if (strlen(line) == 0) // The line is empty
            {
                continue;
            }

            if (strncmp(line, "Price:", 6) == 0)
            {
                sscanf(line, "Price: %f", &local_item->price);
            }
            else if (strncmp(line, "Score:", 6) == 0)
            {
                sscanf(line, "Score: %f", &local_item->score);
            }
            else if (strncmp(line, "Entity:", 7) == 0)
            {
                sscanf(line, "Entity: %d", &local_item->entity);
            }
        }

        sscanf(file_path, "Dataset/Store%c/", &local_item->store_number); // Extract store number
        strcpy(local_item->path, file_path);

        pthread_mutex_lock(&results_array_mutex);
        results[results_count] = *local_item;
        results_count++;
        pthread_mutex_unlock(&results_array_mutex);
    }

    fclose(file);
    free(file_path);
    free(local_item);
    return NULL;
}

int create_log_file(const char *path)
{
    char log_path[1024];
    snprintf(log_path, sizeof(log_path), "%s/log", path);

    struct stat st = {0};
    if (stat(log_path, &st) == -1) // Check if the directory exists
    {
        if (mkdir(log_path, 0700) != 0) // Create the directory
        {
            perror("mkdir failed");
            return EXIT_FAILURE;
        }
    }

    snprintf(log_file_path, sizeof(log_file_path), "%s/%s_Order%d.log", log_path, username, order_number);

    pthread_mutex_lock(&log_file_mutex);
    FILE *file = fopen(log_file_path, "w");
    if (file == NULL)
    {
        printf("%s\n", path);
        perror("Failed to open file //in create_log_file function");
        return EXIT_FAILURE;
    }
    fprintf(file, "Process PID: %d\n", getpid());
    fclose(file);
    pthread_mutex_unlock(&log_file_mutex);

    return EXIT_SUCCESS;
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

    // Read all the category entries(folders & files)
    while ((entry = readdir(dp)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_REG) // Check if it's a regular file
        {
            if (pthread_create(&threads[thread_count], NULL, read_file, (void *)strdup(full_path)) != 0)
            {
                perror("Failed to create thread");
                closedir(dp);
                return;
            }

            // Thread ID for line below
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

pid_t create_process(const char *path)
{
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (dp == NULL)
    {
        perror("Error opening directory");
        return 1;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) // Create a pipe
    {
        perror("Pipe failed");
        closedir(dp);
        return 1;
    }

    pid_t pid = getpid();
    for (int i = 0; i < 8; i++) // Create 8 chlderen for each store
    {
        if (pid != 0)
        {
            do
            {
                entry = readdir(dp);

            } while (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0);

            pid = fork();
        }
    }

    if (pid == 0) // All childeren
    {
        printf("PID %d create child for %s PID: %d \n", getppid(), entry->d_name, getpid());

        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        pthread_mutex_init(&results_array_mutex, NULL);
        pthread_mutex_init(&log_file_mutex, NULL);

        create_log_file(full_path);
        create_thread(full_path);

        pthread_mutex_destroy(&results_array_mutex);
        pthread_mutex_destroy(&log_file_mutex);

        // Send the results array through pipe
        close(pipe_fd[0]); // Close the read end
        for (int i = 0; i < results_count; i++)
        {
            if (results[i].name[0] != '\0')
            {
                write(pipe_fd[1], &results[i], sizeof(item));
            }
        }
        close(pipe_fd[1]); // Close the write end
    }
    else // Parent
    {
        while (wait(NULL) != -1 || errno != ECHILD)
            ; // waits until all the children terminate

        item buffer;

        // Calculating the number of allowed purchases for each product
        close(pipe_fd[1]); // Close the write end
        while (read(pipe_fd[0], &buffer, sizeof(item)) > 0)
        {
            int max_number = 0;

            if (buffer.entity < buffer.number)
            {
                buffer.number = buffer.entity;
            }
            if (price_threshold != FLT_MAX)
            {
                max_number = price_threshold / buffer.price;
                if (max_number < buffer.number)
                {
                    buffer.number = max_number;
                }
                price_threshold -= buffer.price * buffer.number;
            }

            buffer.entity -= buffer.number;
            results[results_count] = buffer;
            results_count++;
        }
        close(pipe_fd[0]); // Close the read end
    }

    closedir(dp);
    return pid;
}

void *order(void *arg)
{
    shop_cart *data = (shop_cart *)arg;
    item *cart = data->cart; // The shopping cart
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
        else if (cart[i].store_number == '2')
        {
            shopping_cart_value[1] += cart[i].score * cart[i].number * 10 / cart[i].price;
        }
        else if (cart[i].store_number == '3')
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
            sprintf(best_store, "The best store is store %d with value = %.3f \n", i + 1, max);
            sprintf(chosen_store, "%d", i + 1);
        }
    }
    printf("%s", best_store);

    return NULL;
}

void *update_files(void *arg)
{
    item *local_item = (item *)arg;

    FILE *file = fopen(local_item->path, "w");
    if (file == NULL)
    {
        perror("Unable to open file!");
        return NULL;
    }

    fprintf(file, "Name: %s\n", local_item->name);
    fprintf(file, "Price: %.2f\n", local_item->price);
    fprintf(file, "Score: %.1f\n", local_item->score);
    fprintf(file, "Entity: %d\n", local_item->entity);
    fprintf(file, "\n");
    fprintf(file, "Last Modified: %s\n", local_item->last_modified);

    fclose(file);
    return NULL;
}

void *score(void *arg)
{
    shop_cart *data = (shop_cart *)arg;
    item *cart = data->cart; // The shopping cart
    int count = data->count;

    while (get_score_lock == true)
        ;
    get_score_lock = true;

    pthread_t thread_id = pthread_self(); // Get the thread ID
    printf("PID %d create thread for Scores TID: %lu \n", main_pid, (unsigned long)thread_id);

    float item_score;

    // Get score & time
    printf("Enter score in range [0, 5] for:\n");
    for (int i = 0; i < count; i++)
    {
        char input[50];

        if (cart[i].store_number != chosen_store[0])
        {
            continue;
        }

        // Get score
        printf("%s: ", cart[i].name);
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
        sscanf(input, "%f", &item_score);

        if (item_score < 0.0 || item_score > 5.0)
        {
            printf("The score entered is not valid !\n");
            i--;
            continue;
        }
        cart[i].score = (cart[i].score + item_score) / 2;

        // Get time (last modified)
        time_t now;
        struct tm *tm_info;

        time(&now);
        tm_info = localtime(&now);

        strftime(cart[i].last_modified, sizeof(cart[i].last_modified), "%Y-%m-%d %H:%M:%S", tm_info);
    }

    // Create thread for updating files
    pthread_t threads[count];
    int thread_count = 0;

    for (int i = 0; i < count; i++)
    {
        if (cart[i].store_number == chosen_store[0])
        {
            pthread_create(&threads[thread_count], NULL, update_files, (void *)&cart[i]);
            thread_count++;
        }
    }

    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return NULL;
}

void * final(void *arg)
{
    shop_cart *data = (shop_cart *)arg;
    item *cart = data->cart; // the shopping cart
    int count = data->count;

    get_score_lock = true;

    pthread_t thread_id = pthread_self(); // Get the thread ID
    printf("PID %d create thread for Final TID: %lu \n", main_pid, (unsigned long)thread_id);

    char file_path[200];
    snprintf(file_path, sizeof(file_path), "Users/%s.txt", username);

    FILE *file = fopen(file_path, "r");
    if (file == NULL)
    {
        perror("Unable to open file");
        return NULL;
    }

    // Reading information from file
    char line[256];
    int store_count[3] = {0, 0, 0};
    int chosen_store_number = atoi(chosen_store);

    while (fgets(line, sizeof(line), file))
    {
        char check_line[256];

        line[strcspn(line, "\n")] = 0;

        int store_number = line[40] - '0';
        int number = line[43] - '0';

        if (store_number == 1)
        {
            store_count[0] = number;
        }
        else if (store_number == 2)
        {
            store_count[1] = number;
        }
        else if (store_number == 3)
        {
            store_count[2] = number;
        }

        snprintf(check_line, sizeof(check_line),
                 "Number of times purchased from the Store%d: %d",
                 chosen_store_number, store_count[chosen_store_number - 1]);

        if (strcmp(line, check_line) == 0)
        {
            store_count[chosen_store_number - 1]++;
        }
    }
    fclose(file);

    // Open file for update user information
    file = fopen(file_path, "w");
    if (file == NULL)
    {
        perror("Unable to open file");
        return NULL;
    }

    // Write information to user file
    fprintf(file, "Username: %s\n", username);
    fprintf(file, "Number of times purchased from the Store1: %d\n", store_count[0]);
    fprintf(file, "Number of times purchased from the Store2: %d\n", store_count[1]);
    fprintf(file, "Number of times purchased from the Store3: %d\n", store_count[2]);
    fclose(file);

    // Calculate the price of the shopping cart for chosen store
    float shopping_cart_price[3] = {0.0, 0.0, 0.0};
    for (int i = 0; i < count; i++)
    {
        if (cart[i].store_number == chosen_store[0])
        {
            shopping_cart_price[chosen_store_number - 1] += cart[i].number * cart[i].price;
        }
    }
    printf("Shopping cart price %d before discount: %.3f\n", chosen_store_number, shopping_cart_price[chosen_store_number - 1]);

    // Apply discount
    if (store_count[chosen_store_number - 1] > 1)
    {
        shopping_cart_price[chosen_store_number - 1] *= 0.9;
    }
    printf("Shopping cart price %d after discount: %.3f\n", chosen_store_number, shopping_cart_price[chosen_store_number - 1]);

    get_score_lock = false;
    return NULL;
}

int main(int argc, char *argv[])
{
    // Receive user order number from server
    order_number = atoi(argv[1]);

    // Read username from users file
    FILE *file = fopen("Users.txt", "r");
    fgets(username, sizeof(username), file);
    username[strcspn(username, "\n")] = 0;
    fclose(file);

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
        printf("PID %d create child for Store%c PID: %d \n", main_pid, store_number, getpid());
        pid_t returned_pid = create_process(store_path);

        if (returned_pid != 0)
        {
            // Send shopping cart data to parent via pipe
            close(pipe_fd[0]); // Close read end
            write(pipe_fd[1], results, sizeof(item) * results_count);
            close(pipe_fd[1]); // Close write end
        }

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
                   all.cart[i].store_number, all.cart[i].name, all.cart[i].price, all.cart[i].score, all.cart[i].entity + all.cart[i].number, all.cart[i].number);
        }

        pthread_t orders_th, scores_th, final_th;
        pthread_create(&orders_th, NULL, order, (void *)&all);
        pthread_create(&scores_th, NULL, score, (void *)&all);
        pthread_create(&final_th, NULL, final, (void *)&all);

        pthread_join(orders_th, NULL);
        pthread_join(scores_th, NULL);
        pthread_join(final_th, NULL);
    }

    return 0;
}