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
#include "unpv13e/lib/unp.h"
#define BUFFER_SIZE 1024

void input_name(int sockfd);
int choose_identity(int sockfd);
void spec_distribute(int sockfd);
void player_distribute(int sockfd);
void handle_game(int sock_fd);
void read_from_server(int sock_fd, char *buffer);
void send_to_server(int sock_fd, const char *message);

int main(int argc, char *argv[]) {
    int					sockfd;
    char                sendline[MAXLINE];
	struct sockaddr_in	servaddr;

	if (argc != 2)
		err_quit("usage: tcpcli <IPaddress>");

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
    printf("Connected to the server.\n");

    // input valid name
    input_name(sockfd);
    snprintf(sendline, sizeof(sendline), "can continue");
    Writen(sockfd, sendline, strlen(sendline));
    
    // choose to be a spectator
    if(choose_identity(sockfd) == 0){
        spec_distribute(sockfd);
    }
    // choose to be a player
    if(choose_identity(sockfd) == 1){
        player_distribute(sockfd);
    }
    
    handle_game(sockfd);

    close(sockfd);
    return 0;
}

void input_name(int sockfd){
    int n;
    char name[100], sendline[MAXLINE], recvline[MAXLINE];
    while(1){
        n = Read(sockfd, recvline, MAXLINE);
        recvline[n] = '\0';
        //printf("Debug: recvline = '%s', length = %zu\n", recvline, strlen(recvline));

        if(strcmp(recvline, "Please input your name: ") == 0 
        || strcmp(recvline, "This name has been used. Please try another name: ") == 0){
            printf("%s", recvline);
            if(Fgets(name, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, name, strlen(name));
            printf("=========================\n");
        }
        else if(strcmp(recvline, "You can end this and go next.") == 0){
            //printf("Are you in?\n");
            break;
        }
        else{
            snprintf(sendline, sizeof(sendline), "ok");
            Writen(sockfd, sendline, strlen(sendline));
            printf("%s", recvline);
        }
    }
}

int choose_identity(int sockfd){
    int n;
    char identity[100], recvline[MAXLINE];
    while(1){
        n = Read(sockfd, recvline, MAXLINE);
        recvline[n] = '\0';
        if(strcmp(recvline, "Do you want to join as a player or a spectator? (player/spectator): ") == 0){
            printf("%s", recvline);
            if(Fgets(identity, MAXLINE, stdin) == NULL) return -1;
            Writen(sockfd, identity, strlen(identity));
        }
        else if(strcmp(recvline, "You can end this and go next.") == 0) break;
        else if(strcmp(recvline, "Invalid input. Please input again.\n") == 0){
            printf("%s", recvline);
        }
    }
    if(strcmp(identity,"player") == 0) return 1;
    else if(strcmp(identity,"spectator") == 0) return 0;
}

void spec_distribute(int sockfd){
    int n;
    char room_id[100], recvline[MAXLINE];
    while(1){
        n = Read(sockfd, recvline, MAXLINE);
        recvline[n] = '\0';
        if(strcmp(recvline, "Please enter the room ID you'd like to spectate: ") == 0){
            printf("%s", recvline);
            if(Fgets(room_id, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, room_id, strlen(room_id));
        }
        else if(strcmp(recvline, "Success!\n") == 0){
            printf("%s", recvline);
            break;
        }
        else if(strcmp(recvline, "This room does not exist or the room has close.\n") == 0){
            printf("%s", recvline);
        }
    }
}

void player_distribute(int sockfd){
    int n;
    char room_id[100], recvline[MAXLINE];
    while(1){
        n = Read(sockfd, recvline, MAXLINE);
        recvline[n] = '\0';
        if(strcmp(recvline, "Do you want to join a specific room? (yes/no): ") == 0){
            printf("%s", recvline);
            if(Fgets(room_id, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, room_id, strlen(room_id));
        }
        else if(strcmp(recvline, "Yes Success!\n") == 0) break;
        else if(strcmp(recvline, "No Success!\n") == 0) break;
        else if(strcmp(recvline, "Invalid input. Please input again.\n") == 0){
            printf("%s", recvline);
        }
    }
}

void handle_game(int sock_fd) {
    char buffer[BUFFER_SIZE];

    while (true) {
        read_from_server(sock_fd, buffer);
        printf("Server: %s", buffer);

        if (strstr(buffer, "enter your expressions") != NULL) {
            printf("Input your expressions (High Bet and Low Bet): ");
            Fgets(buffer, BUFFER_SIZE, stdin);
            send_to_server(sock_fd, buffer);
        } else if (strstr(buffer, "your turn to bet") != NULL) {
            printf("Input your bets (High Bet and Low Bet): ");
            Fgets(buffer, BUFFER_SIZE, stdin);
            send_to_server(sock_fd, buffer);
        } else if (strstr(buffer, "fold") != NULL || strstr(buffer, "out of this round") != NULL) {
            break;
        } else if (strstr(buffer, "Winner") != NULL) {
            printf("Game results: %s", buffer);
            break;
        } else {
            printf("Input your response: ");
            Fgets(buffer, BUFFER_SIZE, stdin);
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
