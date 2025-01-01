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

typedef struct {
    char name[40];
    int number;
}Card;


typedef struct{
    Card cards[4];
    char op[5][2];
}Player;

void input_name(int sockfd);
void choose_identity(int sockfd);
void player_distribute(int sockfd);
void spec_distribute(int sockfd);
void read_from_server(int sock_fd, char *buffer);
void send_to_server(int sock_fd, const char *message);
void receive_card(int sockfd, Player* player);
void handle_bet(int sockfd);
void input_player_combination(int sockfd);


int main(int argc, char *argv[]) {
    int n, maxfdp = 0, sockfd;
    struct sockaddr_in servaddr;
    char sendline[MAXLINE], recvline[MAXLINE], choice[100];
    fd_set rset, recSet;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <Server IP Address>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // initialize
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));
    printf("Connected to the server.\n");
    // send name & identity
    input_name(sockfd);

    snprintf(sendline, sizeof(sendline), "can continue");
    Writen(sockfd, sendline, strlen(sendline));
    sleep(2);

    choose_identity(sockfd);

    FD_ZERO(&recSet);
    FD_SET(sockfd, &recSet);
    maxfdp = sockfd + 1;
    for( ; ; ){
        rset = recSet;
        select(maxfdp, &rset, NULL, NULL, NULL);
        if(FD_ISSET(sockfd, &rset)){
            n = Read(sockfd, recvline, MAXLINE);
            recvline[n] = '\0';
            //printf("Debug: recvline = '%s', length = %zu\n", recvline, strlen(recvline));
            if(strcmp(recvline, "The minimum number of players has been reached. Do you want to start the game now? (yes/no): ") == 0){
                printf("%s", recvline);
                if(Fgets(choice, MAXLINE, stdin) != NULL){
                    Writen(sockfd, choice, strlen(recvline));
                }
            }
            else if(strcmp(recvline, "\nLet start the game.") == 0){
                printf("%s\n", recvline);
                break;
            }
            else if(strcmp(recvline, "Invalid input. Please try again.") == 0){
                printf("%s\n\n", recvline);
            }
        }
    }
    struct Player* player = malloc(sizeof(Player));
    receive_card(sockfd, player);
    // 处理游戏逻辑
    handle_bet(sockfd);
    input_player_combination(sockfd);

    n = Read(sockfd, recvline, MAXLINE);
    recvline[n] = '\0';
    printf("%s", recvline);

    

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
    char choice[100], recvline[MAXLINE];
    while(1){
        n = Read(sockfd, recvline, MAXLINE);
        recvline[n] = '\0';
        //printf("Distribute Debug: recvline = '%s', length = %zu\n", recvline, strlen(recvline));

        if(strcmp(recvline, "Do you want to join a specific room? (yes/no): ") == 0){
            printf("==================================================\n");
            printf("%s", recvline);
            if(Fgets(choice, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, choice, strlen(choice));
        }
        else if(strcmp(recvline, "Yes Success!") == 0){
            break;
        }
        else if(strcmp(recvline, "No Success!") == 0){
            break;
        }
        else if(strcmp(recvline, "Please enter the room ID you'd like to join: ") == 0){
            printf("%s", recvline);
            if(Fgets(choice, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, choice, strlen(choice));
        }
        else if(strcmp(recvline, "Invalid input. Please try again.\n") == 0){
            printf("%s", recvline);
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
        //printf("Debug: recvline = '%s', length = %zu\n", recvline, strlen(recvline));

        if(strcmp(recvline, "Please enter the room ID you'd like to spectate: ") == 0){
            printf("==================================================\n");
            printf("%s", recvline);
            if(Fgets(choice, MAXLINE, stdin) == NULL) return;
            Writen(sockfd, choice, strlen(choice));
        }
        else if(strcmp(recvline, "Spec Success!\n") == 0){
            snprintf(sendline, sizeof(sendline), "I know.");
            Writen(sockfd, sendline, strlen(sendline));
            break;
        }
        else{
            printf("%s", recvline);
        }
    }
}

void receive_card(int sockfd, Player *player) {
    char buffer[BUFFER_SIZE];
    int bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    //printf("Test: %s", buffer);
    if (bytes_received <= 0) {
        perror("Error receiving data");
        return;
    }
    buffer[bytes_received] = '\0';
    printf("%s", buffer);

    // 使用\n作為分隔符，將字串切割成多個部分
    /*char *line = strtok(buffer, "\n");
    int card_index = 0;
    int op_index = 0;

    // 跳過第一行的 "You have been dealt the following cards:"
    if (line) {
        printf("%s\n", line);
        line = strtok(NULL, "\n");
    }

    while (line && card_index < 4){
        char card_name[40];
        int card_number;

        // 使用 sscanf 解析 "Name Number" 格式的每一行
        if (sscanf(line, "%s %d", card_name, &card_number) == 2){
            //printf("card_name: %s, size: %zu\n", card_name, sizeof(player->cards[card_index].name));
            //printf("player: %p, player->cards[%d].name: %p\n", player, card_index, player->cards[card_index].name);
            strncpy(player->cards[card_index].name, card_name, sizeof(player->cards[card_index].name));
            player->cards[card_index].name[sizeof(player->cards[card_index].name) - 1] = '\0';
            player->cards[card_index].number = card_number;
            card_index++;
        }
        line = strtok(NULL, "\n");
    }
    for (int i = 0; i < 4; i++){
        printf("Card %d: %s %d\n", i + 1, player->cards[i].name, player->cards[i].number);
    }

    if(line){
        printf("%s\n", line);
        line = strtok(NULL, "\n");
    }

    while (line && op_index < 4){
        char op;

        // 使用 sscanf 解析 "Name Number" 格式的每一行
        if (sscanf(line, "%c", &op) == 1){
            player->op[op_index][0] = op;
            player->op[op_index][1] = '\0';
            op_index++;
        }
        line = strtok(NULL, "\n");
    }
    for(int i = 0; i < op_index; i++){
        printf("Operator %d: %s\n", i + 1, player->op[i]);
    }*/
}

void handle_bet(int sockfd) {
    int n;
    char sendline[MAXLINE], recvline[BUFFER_SIZE], choice[10], response[MAXLINE];

    while (1) {
        read_from_server(sockfd, recvline);
        printf("%s", recvline);
        // get player's state
        if(Fgets(choice, MAXLINE, stdin) != NULL){
            Writen(sockfd, choice, strlen(choice));
            //else printf("Invalid input. Please try again (0: fold, 1: bet high 2: bet low 3: bet both high and low): ");
        }

        if(strcmp(choice, "0\n") == 0){
            n = Read(sockfd, sizeof(recvline), MAXLINE);
            recvline[n] = '\0';
            printf("%s", recvline);
        }
        if(strcmp(choice, "1\n") == 0 || strcmp(choice, "3\n") == 0){
            n = Read(sockfd, sizeof(recvline), MAXLINE);
            recvline[n] = '\0';
            printf("%s", recvline);
            if(Fgets(sendline, MAXLINE, stdin) != NULL){
                Writen(sockfd, choice, strlen(choice));
            }
            n = Read(sockfd, sizeof(response), MAXLINE);
            response[n] = '\0';
            if(strncmp(response, "Invalid", 7) == 0){
                printf("%s", response);
                continue;
                // 再回到while開頭再來一次
            }
            else if(strncmp(response, "You bet", 7) == 0){
                printf("%s", response);
            }
        }

        if(strcmp(choice, "2\n") == 0 || strcmp(choice, "3\n") == 0){
            n = Read(sockfd, sizeof(recvline), MAXLINE);
            recvline[n] = '\0';
            /*if(strcmp(recvline, "Enter your bet for low: ") == 0){
                printf("%s", recvline);
                if(Fgets(sendline, MAXLINE, stdin) != NULL){
                    Writen(sockfd, choice, strlen(choice));
                }
                n = Read(sockfd, sizeof(response), MAXLINE);
                response[n] = '\0';
                if(strncmp(response, "Invalid", 7) == 0){
                    printf("%s", response);
                    // 再回到while開頭再來一次
                }
                else if(strncmp(response, "You bet", 7) == 0){
                    printf("%s", response);
                }
            }*/
            printf("%s", recvline);
            if(Fgets(sendline, MAXLINE, stdin) != NULL){
                Writen(sockfd, choice, strlen(choice));
            }
            n = Read(sockfd, sizeof(response), MAXLINE);
            response[n] = '\0';
            if(strncmp(response, "Invalid", 7) == 0){
                printf("%s", response);
                continue;
                // 再回到while開頭再來一次
            }
            else if(strncmp(response, "You bet", 7) == 0){
                printf("%s", response);
            }
        }
    }
    }


void input_player_combination(int sockfd){
    int n;
    char sendline[MAXLINE], recvline[BUFFER_SIZE], choice[10], response[MAXLINE];

    while (1){
        read_from_server(sockfd, recvline);
        printf("%s", recvline);
        // get player's state
        if(Fgets(choice, MAXLINE, stdin) != NULL){
            if(strncmp(choice[0], "H", 1) == 0 || strncmp(choice[0], "L", 1) == 0){
                Writen(sockfd, choice, strlen(choice));
                break;
            }
            else{
                n = Read(sockfd, recvline, MAXLINE);
                recvline[n] = '\0';
                printf("%s", recvline);
            }
        }
    }
}



void read_from_server(int sock_fd, char *buffer) {
    memset(buffer, 0, BUFFER_SIZE);
    int bytes_read = Read(sock_fd, buffer, BUFFER_SIZE - 1);
    buffer[bytes_read] = '\0';
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