#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gtk-3.0/gtk/gtk.h>

char username[100];
int order_number = 0;

typedef struct
{
    GtkWidget *entry;
    GtkWidget *text_view;
} AppWidgets;

void login()
{
    // Create user file path
    char file_path[200];
    snprintf(file_path, sizeof(file_path), "Users/%s.txt", username);

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
    else // User file doesn't exists
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

void on_button_clicked(GtkWidget *widget, gpointer data)
{
    AppWidgets *widgets = (AppWidgets *)data;

    // Get text from the entry and copy it to the global username variable
    const char *input_username = gtk_entry_get_text(GTK_ENTRY(widgets->entry));
    strncpy(username, input_username, sizeof(username) - 1);
    username[sizeof(username) - 1] = '\0'; // Ensure null termination

    // Check if username is empty
    if (strlen(username) == 0)
    {
        // Update the text view with the error message
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets->text_view));
        gchar *error_message = "Username cannot be empty!";
        gtk_text_buffer_insert_at_cursor(buffer, error_message, -1);
        gtk_text_buffer_insert_at_cursor(buffer, "\n", -1); // Add a new line
        return;
    }

    login();

    FILE *file = fopen("Users.txt", "w");
    if (file)
    {
        fprintf(file, "%s\n", username);
        fprintf(file, "%d\n", order_number);
        fclose(file);
    }
    else
    {
        g_print("Error writing to Users.txt!\n");
    }

    // Get the current time
    time_t now;
    time(&now);
    struct tm *local_time = localtime(&now);
    char login_time[9]; // HH:MM:SS format
    strftime(login_time, sizeof(login_time), "%H:%M:%S", local_time);

    // Open a new terminal window & transfer the order number
    char command[512];
    // snprintf(command, sizeof(command), "gnome-terminal -- bash -c 'gcc `pkg-config --cflags gtk+-3.0` -o client client.c `pkg-config --libs gtk+-3.0` && ./client; exec bash'");
    snprintf(command, sizeof(command), "gnome-terminal -- bash -c 'gcc client.c -o client && ./client %d; exec bash'", order_number);

    if (system(command) == -1)
    {
        g_print("Error executing command!\n");
    }

    // Update the text view with the new login info
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets->text_view));
    gchar *new_entry = g_strdup_printf("Username: %s, Login Time: %s", username, login_time);
    gtk_text_buffer_insert_at_cursor(buffer, new_entry, -1);
    gtk_text_buffer_insert_at_cursor(buffer, "\n", -1); // Add a new line
    g_free(new_entry);

    // Clear the entry field
    gtk_entry_set_text(GTK_ENTRY(widgets->entry), ""); // Clear the entry
}

int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *entry;
    GtkWidget *button;
    GtkWidget *box;
    GtkWidget *text_view;

    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create a new window
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Enter your username: ");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);

    // Create a vertical box to pack widgets
    box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), box);

    // Create a text view for displaying user info
    text_view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE); // Make it read-only
    gtk_box_pack_start(GTK_BOX(box), text_view, TRUE, TRUE, 0);

    // Create an entry widget
    entry = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(box), entry, TRUE, TRUE, 0);

    // Create a button
    button = gtk_button_new_with_label("Submit");
    gtk_box_pack_start(GTK_BOX(box), button, TRUE, TRUE, 0);

    // Create an AppWidgets struct to hold references
    AppWidgets widgets = {entry, text_view};

    // Connect signals
    g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), &widgets);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // Show all widgets
    gtk_widget_show_all(window);

    // Start the GTK main loop
    gtk_main();

    return 0;
}