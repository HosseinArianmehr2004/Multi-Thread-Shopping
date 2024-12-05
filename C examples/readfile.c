#include <stdio.h>
#include <stdlib.h>

void readLines(const char *filePath) {
    FILE *file = fopen(filePath, "r");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    char buffer[256]; // Buffer to hold each line

    // Read lines until fgets returns NULL (end of file)
    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("%s", buffer); // Print the line read
    }

    fclose(file);
}

int main() {
    char filePath[1024];

    printf("Enter the file path to read: ");
    scanf("%1023s", filePath);

    readLines(filePath); // Call the function to read lines from the file

    return EXIT_SUCCESS;
}
