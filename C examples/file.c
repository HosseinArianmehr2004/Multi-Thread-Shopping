#include <stdio.h>
#include <stdlib.h>

int main() {
    // Pointer to a FILE object
    FILE *file;

    // Create and open a new file for writing ("w" mode)
    file = fopen("example.txt", "w");

    // Check if the file was created successfully
    if (file == NULL) {
        perror("Error creating file");
        return EXIT_FAILURE;
    }

    // Write text to the file
    fprintf(file, "Hello, World!\n");
    fprintf(file, "This is an example of file creation in C.\n");

    // Close the file
    fclose(file);

    printf("File created and written successfully.\n");

    return EXIT_SUCCESS;
}
