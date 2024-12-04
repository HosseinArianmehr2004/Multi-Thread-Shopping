#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main()
{
    char username[100]; // an array of chars is an string :/

    printf("Username: ");
    scanf("%99s", username);

    char file_path[200];
    strcpy(file_path, "Users/");
    strcat(file_path, username);
    strcat(file_path, ".log");

    FILE *file;
    file = fopen(file_path, "w");
    if (file == NULL) {
        perror("Error creating file");
        return EXIT_FAILURE;
    }

    char order_list[100][50]; // Array with 100 rows of strings, each with length 50
    printf("Orderlist0:\n");

    char input[50];
    int i = 0;
    while (i < 100)
    {
        scanf("%49s", input);

        if (strcmp(input, "done") == 0)
            break;

        strcpy(order_list[i], input);
        i++;
    }

    int threshold;
    printf("Price threshold: ");
    scanf("%d", &threshold);

    return 0;
}
