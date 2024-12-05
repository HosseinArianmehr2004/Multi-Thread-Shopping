#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

void processDirectory(const char *path) {
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (dp == NULL) {
        perror("Error opening directory");
        return;
    }

    while ((entry = readdir(dp)) != NULL) {
        // Skip the current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct the full path
        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

        // Use stat to check if the entry is a directory
        struct stat statbuf;
        if (stat(fullPath, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
            pid_t pid = fork();
            if (pid == 0) {
                // Child process
                printf("Processing directory: %s\n", fullPath);
                // Recursively process the subdirectory
                processDirectory(fullPath);
                exit(0);
            } else if (pid > 0) {
                // Parent process
                wait(NULL); // Wait for the child process to finish
            } else {
                perror("Fork failed");
            }
        }
    }

    closedir(dp);
}

int main() {
    char directory[1024];

    printf("Enter the directory path to process: ");
    scanf("%1023s", directory);

    processDirectory(directory);

    return 0;
}


void processDirectory(const char *path) {
    struct dirent *entry;
    DIR *dp = opendir(path);

    if (dp == NULL) {
        perror("Error opening directory");
        return;
    }

    while ((entry = readdir(dp)) != NULL) {
        // Skip the current and parent directory entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Construct the full path
        char fullPath[1024];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);

        // Check if the entry is a directory
        if (entry->d_type == DT_DIR) {
            pid_t pid = fork();
            if (pid == 0) {
                // Child process
                printf("Processing directory: %s\n", fullPath);
                // You can add any processing logic here
                // For this example, we just exit the child process
                exit(0);
            } else if (pid > 0) {
                // Parent process
                wait(NULL); // Wait for the child process to finish
            } else {
                perror("Fork failed");
            }
        }
    }

    closedir(dp);
}

