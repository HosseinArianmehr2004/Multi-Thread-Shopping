#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

typedef struct {
    char store_number;
    char name[50];
    float price;
    float score;
    int entity;
    int number;
} item;

typedef struct {
    item *cart;
    int count;
} order_data;

void *order(void *arg) {
    order_data *data = (order_data *)arg; // Cast to the correct type
    item *shopping_cart = data->cart; // Access the shopping cart
    int shopping_cart_count = data->count;

    pthread_t thread_id = pthread_self();
    printf("PID %d create thread for Orders TID: %lu \n", getpid(), (unsigned long)thread_id);

    // Process the shopping cart items
    for (int i = 0; i < shopping_cart_count; i++) {
        printf("Processing item from Store %c: %s: Price = %.2f\n",
               shopping_cart[i].store_number, shopping_cart[i].name, shopping_cart[i].price);
    }

    free(data); // Free the allocated memory for data
    return NULL;
}

int main() {
    // Your existing initialization code...

    // After collecting all shopping carts
    item all_shopping_carts[300];
    int total_items = 0;

    // Assuming you have filled all_shopping_carts and total_items correctly

    // Create a structure to hold the cart data
    order_data *data = malloc(sizeof(order_data));
    data->cart = all_shopping_carts;
    data->count = total_items;

    pthread_t orders_th, scores_th, final_th;
    pthread_create(&orders_th, NULL, order, data); // Pass the data struct to the thread

    // Create other threads as needed
    // pthread_create(&scores_th, NULL, score, NULL);
    // pthread_create(&final_th, NULL, final, NULL);

    // Join the threads
    pthread_join(orders_th, NULL);
    // pthread_join(scores_th, NULL);
    // pthread_join(final_th, NULL);

    return 0;
}
