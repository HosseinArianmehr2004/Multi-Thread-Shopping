#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char username[100];
int order_number;

void login()
{
    // Get username
    printf("Enter your username: ");
    fgets(username, sizeof(username), stdin);
    username[strcspn(username, "\n")] = 0;

    // Create user file path
    char file_path[200];
    snprintf(file_path, sizeof(file_path), "Users/%s.txt", username);

    // Checking for the existence of the user file
    FILE *file = fopen(file_path, "r");

    if (file) // User file exists
    {
        // Calculate the number of previous purchases made by the user
        char line[256];
        int store_count[3] = {0, 0, 0};

        while (fgets(line, sizeof(line), file))
        {
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
        }

        order_number = store_count[0] + store_count[1] + store_count[2];
    }
    else // User file not exists
    {
        // Create user file
        file = fopen(file_path, "w");

        // Writing information in file
        fprintf(file, "Username: %s\n", username);
        fprintf(file, "Number of times purchased from the Store1: %d\n", 0);
        fprintf(file, "Number of times purchased from the Store2: %d\n", 0);
        fprintf(file, "Number of times purchased from the Store3: %d\n", 0);
    }
    fclose(file);
}

int main()
{
    login();

    // Write username in users file
    FILE *file = fopen("users.txt", "w");
    fprintf(file, "%s\n", username);
    fclose(file);

    // Open a new terminal window & transfer the order number
    char command[100];
    snprintf(command, sizeof(command),
             "gnome-terminal -- bash -c 'gcc client.c -o client && ./client %d; exec bash'",
             order_number);
    system(command);

    return 0;
}