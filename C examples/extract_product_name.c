#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_NAME_LENGTH 100

int main()
{
    FILE *file;
    char line[256];
    char name[MAX_NAME_LENGTH];

    // Open the file for reading
    file = fopen("../Dataset/Store1/Food/240.txt", "r");
    if (file == NULL)
    {
        perror("Unable to open file");
        return 1;
    }

    // Read lines until we find the name
    while (fgets(line, sizeof(line), file))
    {
        if (sscanf(line, "Name: %99[^\n]", name) == 1)
        {
            // Successfully read the name
            break;
        }
    }

    // Close the file
    fclose(file);

    // Print the name
    printf("Name: %s\n", name);

    return 0;
}
