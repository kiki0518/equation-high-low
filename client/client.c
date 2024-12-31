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
#include "unpv13e/lib/unp.h"

#define BUFFER_SIZE 1024

void input_name(int sockfd);
void choose_identity(int sockfd);
void player_distribute(int sockfd);
void spec_distribute(int sockfd);
void check_start(int sockfd);
void read_from_server(int sock_fd, char *buffer);
void send_to_server(int sock_fd, const char *message);
void handle_game(int sock_fd);


int main(int argc, char *argv[]) {
    int n, sockfd;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Server IP Address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

    printf("Connected to the server.\n");

    input_name(sockfd);
    choose_identity(sockfd);

    check_start(sockfd);
    // 处理游戏逻辑
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
        //printf("Debug: recvline = '%s', length = %zu\n", recvline, strlen(recvline));

        if(strcmp(recvline, "Please input your name: ") == 0 ||
           strcmp(recvline, "This name has been used. Please try another name: ") == 0){
            printf("%s", recvline);
            if(Fgets(name, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, name, strlen(name));
        }
        else if(strcmp(recvline, "You can go next.") == 0){
            break;
        }
        else{
            printf("%s", recvline);
        }
    }
}

void choose_identity(int sockfd){
    int n;
    char identity[100], sendline[MAXLINE], recvline[MAXLINE];
    while(1){
        n = Read(sockfd, recvline, MAXLINE);
        recvline[n] = '\0';
        //printf("Debug: recvline = '%s', length = %zu\n", recvline, strlen(recvline));

        if(strcmp(recvline, "Do you want to join as a player or a spectator? (player/spectator): ") == 0){
            printf("==================================================\n");
            printf("%s", recvline);
            if(Fgets(identity, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, identity, strlen(identity));
        }
        else if(strcmp(recvline, "You can end this and go next.") == 0 && strcmp(identity,"player\n") == 0){
            snprintf(sendline, sizeof(sendline), "I get.");
            Writen(sockfd, sendline, strlen(sendline));
            player_distribute(sockfd);
            break;
        }
        else if(strcmp(recvline, "You can end this and go next.") == 0 && strcmp(identity,"spectator\n") == 0){
            snprintf(sendline, sizeof(sendline), "I get.");
            Writen(sockfd, sendline, strlen(sendline));
            spec_distribute(sockfd);
            break;
        }
        else{
            printf("%s", recvline);
        }
    }
}

void player_distribute(int sockfd){
    int n;
    char choice[100], sendline[MAXLINE], recvline[MAXLINE];
    while(1){
        n = Read(sockfd, recvline, MAXLINE);
        recvline[n] = '\0';
        //printf("Debug: recvline = '%s', length = %zu\n", recvline, strlen(recvline));

        if(strcmp(recvline, "Do you want to join a specific room? (yes/no): ") == 0){
            printf("==================================================\n");
            printf("%s", recvline);
            if(Fgets(choice, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, choice, strlen(choice));
        }
        else if(strcmp(recvline, "Yes Success!\n") == 0){
            snprintf(sendline, sizeof(sendline), "I know.");
            Writen(sockfd, sendline, strlen(sendline));
            break;
        }
        else if(strcmp(recvline, "No Success!\n") == 0){
            snprintf(sendline, sizeof(sendline), "I know.");
            Writen(sockfd, sendline, strlen(sendline));
            break;
        }
        else if(strcmp(recvline, "Please enter the room ID you'd like to join: ") == 0){
            printf("%s", recvline);
            if(Fgets(choice, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, choice, strlen(choice));
        }
        else{
            printf("%s", recvline);
        }
    }
}

void spec_distribute(int sockfd){
    int n;
    char choice[100], sendline[MAXLINE], recvline[MAXLINE];
    while(1){
        n = Read(sockfd, recvline, MAXLINE);
        recvline[n] = '\0';
        printf("Debug: recvline = '%s', length = %zu\n", recvline, strlen(recvline));

        if(strcmp(recvline, "Please enter the room ID you'd like to spectate: ") == 0){
            printf("==================================================\n");
            printf("%s", recvline);
            if(Fgets(choice, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, choice, strlen(choice));
        }
        else if(strcmp(recvline, "Yes Success!\n") == 0){
            snprintf(sendline, sizeof(sendline), "I know.");
            Writen(sockfd, sendline, strlen(sendline));
            break;
        }
        else if(strcmp(recvline, "No Success!\n") == 0){
            snprintf(sendline, sizeof(sendline), "I know.");
            Writen(sockfd, sendline, strlen(sendline));
            break;
        }
        else if(strcmp(recvline, "Please enter the room ID you'd like to join: ") == 0){
            printf("%s", recvline);
            if(Fgets(choice, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, choice, strlen(choice));
        }
        else{
            printf("%s", recvline);
        }
    }
}

void check_start(int sockfd){
    int n;
    char choice[100], sendline[MAXLINE], recvline[MAXLINE];
    while(1){
        n = Read(sockfd, recvline, MAXLINE);
        recvline[n] = '\0';
        //printf("Debug: recvline = '%s', length = %zu\n", recvline, strlen(recvline));

        if(strcmp(recvline, "The minimum number of players has been reached. Do you want to start the game now? (yes/no): ") == 0){
            //printf("==================================================\n");
            printf("%s", recvline);
            if(Fgets(choice, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, choice, strlen(choice));
        }
        else if(strcmp(recvline, "Let start the game.") == 0){
            sleep(2);
            break;
        }
    }
}

void handle_game(int sock_fd) {
    char buffer[BUFFER_SIZE];
    char input[BUFFER_SIZE];

    while (1) {
        // 从服务端读取消息
        read_from_server(sock_fd, buffer);
        printf("Server: %s\n", buffer);

        if (strstr(buffer, "你的数学表达式是:") != NULL) {
            printf("Input your guess (or type 'fold' to quit): ");
            Fgets(input, BUFFER_SIZE, stdin);
            send_to_server(sock_fd, input);
        } else if (strstr(buffer, "筹码耗尽") != NULL || strstr(buffer, "游戏结束") != NULL) {
            printf("Game Over! Disconnecting...\n");
            break;
        } else {
            printf("Input your response: ");
            Fgets(input, BUFFER_SIZE, stdin);
            send_to_server(sock_fd, input);
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
