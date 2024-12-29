/*
Main client logic that handles:
Connecting to the server.
Sending player actions (e.g., name, bets, and guesses).
Receiving game updates from the server.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

void connect_to_server(const char *server_ip, int port, int *sock_fd);
void handle_game(int sock_fd);
void read_from_server(int sock_fd, char *buffer);
void send_to_server(int sock_fd, const char *message);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sock_fd;
    connect_to_server(server_ip, port, &sock_fd);

    printf("Connected to the server.\n");
    handle_game(sock_fd);

    close(sock_fd);
    return 0;
}

void connect_to_server(const char *server_ip, int port, int *sock_fd) {
    *sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (*sock_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid server IP address");
        close(*sock_fd);
        exit(EXIT_FAILURE);
    }

    if (connect(*sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection to the server failed");
        close(*sock_fd);
        exit(EXIT_FAILURE);
    }
}

void handle_game(int sock_fd) {
    char buffer[BUFFER_SIZE];

    while (true) {
        read_from_server(sock_fd, buffer);
        printf("Server: %s", buffer);

        if (strstr(buffer, "enter your expressions") != NULL) {
            printf("Input your expressions (High Bet and Low Bet): ");
            fgets(buffer, BUFFER_SIZE, stdin);
            send_to_server(sock_fd, buffer);
        } else if (strstr(buffer, "your turn to bet") != NULL) {
            printf("Input your bets (High Bet and Low Bet): ");
            fgets(buffer, BUFFER_SIZE, stdin);
            send_to_server(sock_fd, buffer);
        } else if (strstr(buffer, "fold") != NULL || strstr(buffer, "out of this round") != NULL) {
            break;
        } else if (strstr(buffer, "Winner") != NULL) {
            printf("Game results: %s", buffer);
            break;
        } else {
            printf("Input your response: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            send_to_server(sock_fd, buffer);
        }
    }
}

void read_from_server(int sock_fd, char *buffer) {
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_read = read(sock_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read <= 0) {
        perror("Failed to read from server");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
}

void send_to_server(int sock_fd, const char *message) {
    if (write(sock_fd, message, strlen(message)) < 0) {
        perror("Failed to send to server");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
}
