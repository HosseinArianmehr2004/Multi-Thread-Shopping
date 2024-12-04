#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char username[100];       // an array of chars is an string :/
char order_list[100][50]; // Array with 100 rows of strings, each with length 50

void login()
{
    printf("Username: ");
    scanf("%99s", username);

    char file_path[200];
    strcpy(file_path, "Users/");
    strcat(file_path, username);
    strcat(file_path, ".txt");

    FILE *file;
    file = fopen(file_path, "w");
    if (file == NULL)
    {
        perror("Error creating file");
        // return EXIT_FAILURE;
    }
    fprintf(file, "Username: %s\n", username);
    fprintf(file, "Purchase Times: %d", 0);
    fclose(file);
}

void get_order_list()
{
    printf("Orderlist0:\n");

    char input[50];
    for (int i = 0; i < 100; i++)
    {
        scanf("%49s", input);

        if (strcmp(input, "done") == 0)
            break;

        strcpy(order_list[i], input);
    }

    int threshold;
    printf("Price threshold: ");
    scanf("%d", &threshold);
}

int main()
{
    // login();
    // get_order_list();

    int pid1 = fork();
    int pid2 = fork();

    if (pid1 != 0 && pid2 != 0)
    {
        // Parent process
        printf("parent %d %d\n", pid1, pid2);
    }
    else
    {
        // All 3 childs and grandchilds
        printf("child %d %d\n", pid1, pid2);
    }

    return 0;
}
