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
void handle_game(int sock_fd);
void read_from_server(int sock_fd, char *buffer);
void send_to_server(int sock_fd, const char *message);

int main(int argc, char *argv[]) {
    int					n, sockfd;
	struct sockaddr_in	servaddr;
    char                sendline[MAXLINE], recvline[MAXLINE], name[100];

	if (argc != 2)
		err_quit("usage: tcpcli <IPaddress>");

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT+5);
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
    printf("Connected to the server.\n");

    // input valid name
    input_name(sockfd);
    
    // choose to be a spectator
    if(choose_identity(sockfd) == 0){
        
    }
    if(choose_identity(sockfd) == 1){
        
    }


    
    handle_game(sockfd);

    close(sockfd);
    return 0;
}

void input_name(int sockfd){
    int n;
    char name[100], recvline[MAXLINE];
    while(1){
        n = Read(sockfd, recvline, MAXLINE);
        recvline[n] = '\0';
        if(recvline == "Please input your name: " || recvline == "This name has been used. Please try another name: "){
            printf("%s", recvline);
            if(Fget(name, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, name, strlen(name));
        }
        else if(recvline == "end") break;
        else printf("%s", recvline);
    }
}

int choose_identity(int sockfd){
    int n;
    char identity[100], recvline[MAXLINE];
    while(1){
        n = Read(sockfd, recvline, MAXLINE);
        recvline[n] = '\0';
        if(recvline == "Do you want to join as a player or a spectator? (player/spectator): "){
            printf("%s", recvline);
            if(Fget(identity, MAXLINE, stdin) == NULL) return -1;
            Writen(sockfd, identity, strlen(identity));
        }
        else if(recvline == "end") break;
        else if(recvline == "Invalid input. Please input again.\n"){
            printf("%s", recvline);
        }
    }
    if(identity == "player") return 1;
    if(identity == "spectator") return 0;
}

void spec_distribute(int sockfd){
    int n;
    char room_id[100], recvline[MAXLINE];
    while(1){
        n = Read(sockfd, recvline, MAXLINE);
        recvline[n] = '\0';
        if(recvline == "Please enter the room ID you'd like to spectate: "){
            printf("%s", recvline);
            if(Fget(room_id, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, room_id, strlen(room_id));
        }
        else if(recvline == "Success!\n"){
            printf("%s", recvline);
            break;
        }
        else if(recvline == "This room does not exist or the room has close.\n"){
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
