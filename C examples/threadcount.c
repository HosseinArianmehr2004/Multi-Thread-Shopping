#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>

// Function that each thread will execute
void* process_file(void* arg) {
    char* filename = (char*)arg;
    printf("Processing file: %s\n", filename);
    // Simulate file processing
    sleep(1);
    return NULL;
}

int main() {
    DIR* dir;
    struct dirent* entry;
    pthread_t threads[100]; // Assume a maximum of 100 files
    int thread_count = 0;

    // Open the directory
    dir = opendir("../Dataset/Store1/Digital"); // Change to your directory path
    if (dir == NULL) {
        perror("opendir");
        return EXIT_FAILURE;
    }

    // Read each file in the directory
    while ((entry = readdir(dir)) != NULL) {
        // Skip the current and parent directory entries
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            // Create a new thread for each file
            if (pthread_create(&threads[thread_count], NULL, process_file, entry->d_name) != 0) {
                perror("pthread_create");
                return EXIT_FAILURE;
            }
            thread_count++;
        }
    }

    // Close the directory
    closedir(dir);

    // Wait for all threads to finish
    for (int i = 0; i < thread_count; i++) {
        pthread_join(threads[i], NULL);
    }

    return EXIT_SUCCESS;
}
