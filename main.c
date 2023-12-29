#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define PORT 8083
#define BUFFER_SIZE 1024
#define DEFAULT_FILE "index.html"

void send_response(int client_socket, const char *content_type, const char *content)
{
    dprintf(client_socket, "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n%s", content_type, content);
}

void *handle_request(void *arg)
{
    int client_socket = *((int *)arg);
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);

    if (bytes_received <= 0)
    {
        perror("Error reading from socket");
        close(client_socket);
        pthread_exit(NULL);
    }

    // Parse the HTTP request (you can implement your own parser or use existing libraries)
    // Extract the requested file path from the request (very basic parsing)
    char requested_file[BUFFER_SIZE];
    sscanf(buffer, "GET /%s ", requested_file);

    // If the requested file is empty, serve the default file
    if (strlen(requested_file) == 0)
    {
        strcpy(requested_file, DEFAULT_FILE);
    }

    // Open the requested file
    FILE *file = fopen(requested_file, "r");
    if (file == NULL)
    {
        perror("Error opening file");
        close(client_socket);
        pthread_exit(NULL);
    }

    // Determine the content type based on the file extension
    const char *content_type = "text/plain";
    if (strstr(requested_file, ".html"))
    {
        content_type = "text/html";
    }
    else if (strstr(requested_file, ".css"))
    {
        content_type = "text/css";
    }
    else if (strstr(requested_file, ".js"))
    {
        content_type = "application/javascript";
    }

    // Read the contents of the file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = (char *)malloc(file_size + 1);
    fread(file_content, 1, file_size, file);
    fclose(file);

    file_content[file_size] = '\0';

    // Send the HTTP response with the file content
    send_response(client_socket, content_type, file_content);

    free(file_content);
    close(client_socket);
    pthread_exit(NULL);
}

int main()
{
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_socket, 10) == -1)
    {
        perror("Error listening for connections");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1)
    {
        // Accept a connection
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == -1)
        {
            perror("Error accepting connection");
            continue;
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Create a new thread to handle the request
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_request, (void *)&client_socket) != 0)
        {
            perror("Error creating thread");
            close(client_socket);
        }
        else
        {
            // Detach the thread to clean up resources automatically
            pthread_detach(thread);
        }
    }

    // Close the server socket (never reaches this point in the example)
    close(server_socket);

    return 0;
}
