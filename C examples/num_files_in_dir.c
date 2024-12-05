#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

int countFilesInDirectory(const char *path) {
    struct dirent *entry;
    DIR *dp = opendir(path);
    int fileCount = 0;

    if (dp == NULL) {
        perror("Error opening directory");
        return 0;
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
        if (stat(fullPath, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                // If it's a directory, count files recursively
                fileCount += countFilesInDirectory(fullPath);
            } else {
                // It's a file; increment the count
                fileCount++;
            }
        }
    }

    closedir(dp);
    return fileCount;
}

int main() {
    char directory[1024];

    printf("Enter the directory path to count files: ");
    scanf("%1023s", directory);

    int totalFiles = countFilesInDirectory(directory);
    printf("Total number of files in directory '%s': %d\n", directory, totalFiles);

    return 0;
}
