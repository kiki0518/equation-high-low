/*
Main server logic that handles:
Client connections.
Game phases (e.g., preparation, gameplay, result broadcast).
Communication with clients over TCP sockets.
Integrates game logic and manages player state.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "unpv13e/lib/unp.h"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

// Room structure to manage multiple rooms
#define MAX_PLAYERS 100
#define MAX_ROOMS 10
#define ROOM_CAPACITY 5
#define MIN_START_PLAYERS 3
// unpv13e/lib/unp.h

typedef struct {
    int player_count;
    int connfd[MAX_PLAYERS];
    char name[MAX_PLAYERS][100];
    int room_id;
    int host_id; // The player ID of the room host
} Room;

Room rooms[MAX_ROOMS]; // Manage multiple rooms
int room_count = 0; // Number of active rooms

void sig_chld(int signo){
    pid_t pid;
    int   stat;
    while((pid = waitpid(-1, &stat, WNOHANG)) > 0);
    // -1: wait for the first terminated child
	// WNOHANG: not block if there are no more terminated children
    return;
}

void letPlay(Room* room, int listenfd) {
    char sendline[MAXLINE];
    snprintf(sendline, sizeof(sendline), "Game in Room %d starts now!\n", room->room_id);
    for (int i=0; i < room->player_count; ++i) {
        Writen(room->connfd[i], sendline, strlen(sendline));
    }

    // Implement the game logic here...
    // 參數傳入
    char arguments[MAX_PLAYERS][300];
    char *args[MAX_PLAYERS+2];
    strcpy(arguments[0],"./game");
    args[0] = arguments[0];
    for(int i=0; i<room->player_count; i++){
        sprintf(arguments[i+1],"%d",room->connfd[i]);
        args[i+1] = arguments[i+1];
    }
    execv("./dealer",args);

    sleep(5); // Simulate game duration
    snprintf(sendline, sizeof(sendline), "Game in Room %d ends.\n", room->room_id);
    for (int i = 0; i < room->player_count; ++i) {
        Writen(room->connfd[i], sendline, strlen(sendline));
        Close(room->connfd[i]);
    }
    room->player_count = 0; // Reset the room
}

int checkRoomAvailability(int room_id) {
    for (int i = 0; i < room_count; i++) {
        if (rooms[i].room_id == room_id && rooms[i].player_count < ROOM_CAPACITY) {
            return i; // 返回房間的索引
        }
    }
    return -1; // 沒有找到符合條件的房間
}

int assignToRoom(int connfd, char* name, int listenfd) {
    for (int i=0; i<room_count; i++) {
        if (rooms[i].player_count < ROOM_CAPACITY) {
            int player_id = rooms[i].player_count;
            rooms[i].connfd[player_id] = connfd;
            strcpy(rooms[i].name[player_id], name);
            rooms[i].player_count++;

            char sendline[MAXLINE];
            snprintf(sendline, sizeof(sendline), "You are added to Room %d as player #%d.\n", rooms[i].room_id, player_id + 1);
            Writen(connfd, sendline, strlen(sendline));

            // Check if game should start
            if (rooms[i].player_count == MIN_START_PLAYERS){
                snprintf(sendline, sizeof(sendline), "The minimum number of players (%d) has been reached. Do you want to start the game now? (yes/no): ", MIN_START_PLAYERS);
                Writen(rooms[i].connfd[0], sendline, strlen(sendline));

                // 設置超時等待房主回應
                fd_set readfds;
                FD_ZERO(&readfds);
                FD_SET(rooms[i].connfd[0], &readfds);
                struct timeval timeout = {30, 0};  // 30秒超時

                int activity = select(rooms[i].connfd[0] + 1, &readfds, NULL, NULL, &timeout);
                if (activity > 0 && FD_ISSET(rooms[i].connfd[0], &readfds)) {
                    // 讀取房主的回應
                    char response[100];
                    int n = Read(rooms[i].connfd[0], response, sizeof(response) - 1);
                    response[n] = '\0';

                    if (strcasecmp(response, "yes") == 0){
                        // 房主選擇開始遊戲
                        printf("Room %d: The host has started the game.\n", rooms[i].room_id);
                        pid_t pid = fork();
                        if (pid == 0) {
                            Close(listenfd);
                            letPlay(&rooms[i], listenfd);
                            exit(0);
                        }
                    } 
                    else{
                        // 房主選擇不開始，繼續等待其他玩家加入
                        printf("Room %d: The host chose not to start the game yet.\n", rooms[i].room_id);
                    }
                } 
                else{
                    // 超時未回應，繼續等待玩家加入
                    printf("Room %d: The host did not respond in time. Waiting for more players...\n", rooms[i].room_id);
                }
            } 
            else if(rooms[i].player_count == ROOM_CAPACITY) {
                letPlay(&rooms[i], listenfd);
            }
            return rooms[i].room_id;
        }
    }

    // Create a new room if no room is available
    if (room_count < MAX_ROOMS){
        Room* new_room = &rooms[room_count++];
        new_room->room_id = room_count;        // start from 1
        new_room->player_count = 1;
        new_room->connfd[0] = connfd;
        strcpy(new_room->name[0], name);
        new_room->host_id = 0; // First player becomes the host

        char sendline[MAXLINE];
        snprintf(sendline, sizeof(sendline), "You are the host of Room %d.\n", new_room->room_id);
        Writen(connfd, sendline, strlen(sendline));

        return new_room->room_id;
    }

    // No room available
    char sendline[MAXLINE];
    snprintf(sendline, sizeof(sendline), "Sorry, no rooms available.\n");
    Writen(connfd, sendline, strlen(sendline));
    Close(connfd);
    return -1;
}

int main(){
    int listenfd, maxfdp;
    int n, total_id, connfd[100];
    char name[100][100], sendline[MAXLINE];
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    fd_set rset, recSet;

    // create listen socket
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // port number set?
    servaddr.sin_port = htons(SERV_PORT+3);

    // can reuse ip address
    int optval = 1; 
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    Bind(listenfd, (SA*) &servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);
    Signal(SIGCHLD, sig_chld);

    // initiate
    total_id = 0;
    FD_ZERO(&recSet);
    FD_SET(listenfd, &recSet);
    maxfdp = listenfd + 1;
    while(1){
        rset = recSet;
        select(maxfdp, &rset, NULL, NULL, NULL);
        // accept client
        if(FD_ISSET(listenfd, &rset)){
            // up to 100 ??? 我們有很多房間w
            if(total_id <= 100){
                // total_id: 從server開啟開始算起的第幾位client 
                // start from 1
                total_id++;
                clilen = sizeof(cliaddr);
                connfd[total_id] = Accept(listenfd, (SA*) &cliaddr, &clilen);
                printf("\n========= Client %d connected. =========\n", total_id);
                FD_SET(connfd[total_id], &recSet);
                if(connfd[total_id]+1 > maxfdp){
                    maxfdp = connfd[total_id] + 1;
                }
                // ask client's name + receive name
                int ok = 0, repeat = 0;
                snprintf(sendline, sizeof(sendline), "Please input your name: ");
                Writen(connfd[total_id], sendline, strlen(sendline));
                // if name is same as other player... use id to distinguish 
                // or ask players to input again (below)
                while(ok == 0){
                    n = Read(connfd[total_id], name[total_id], 100);
                    name[total_id][n] = '\0';
                    printf("Recv: %s\n", name[total_id]);
                    for(int i=0; i<total_id; i++){
                        if(strcmp(name[total_id], name[i]) == 0){
                            repeat = 1;
                            break;
                        }
                    }
                    if(repeat == 0) ok = 1;
                    else{
                        snprintf(sendline, sizeof(sendline), "This name has been used. Please try another name: ");
                        Writen(connfd[total_id], sendline, strlen(sendline));
                        repeat = 0;
                    }
                }
                // online_id: 這個房間的第幾位
                snprintf(sendline, sizeof(sendline), "You are the #%d user.\n", total_id);
                Writen(connfd[total_id], sendline, strlen(sendline));
                snprintf(sendline, sizeof(sendline), "You may now play with computer or wait for other users.\n", total_id);
                Writen(connfd[total_id], sendline, strlen(sendline));
                printf("Send: %s is the #%d user.\n", name[total_id], total_id);

                // Assign player to a room
                int selected_room_id = -1;
                char input[100];

                // 問玩家是否要選擇特定房間
                snprintf(sendline, sizeof(sendline), "Do you want to join a specific room? (yes/no): ");
                Writen(connfd[total_id], sendline, strlen(sendline));
                n = Read(connfd[total_id], input, sizeof(input) - 1);
                input[n] = '\0';

                if (strcasecmp(input, "yes") == 0) {
                    snprintf(sendline, sizeof(sendline), "Please enter the room ID you'd like to join: ");
                    Writen(connfd[total_id], sendline, strlen(sendline));
                    n = Read(connfd[total_id], input, sizeof(input) - 1);
                    input[n] = '\0';
                    selected_room_id = atoi(input); // 將輸入轉換為整數房間ID

                    // 檢查房間是否存在以及是否有空位
                    int room_index = checkRoomAvailability(selected_room_id);
                    if (room_index != -1) {
                        // 房間存在且有空位，加入該房間
                        int player_id = rooms[room_index].player_count;
                        rooms[room_index].connfd[player_id] = connfd[total_id];
                        strcpy(rooms[room_index].name[player_id], name[total_id]);
                        rooms[room_index].player_count++;

                        snprintf(sendline, sizeof(sendline), "You are added to Room %d as player #%d.\n", rooms[room_index].room_id, player_id + 1);
                        Writen(connfd[total_id], sendline, strlen(sendline));

                        // 如果玩家達到起始條件，進行遊戲處理
                        if (rooms[room_index].player_count == MIN_START_PLAYERS) {
                            snprintf(sendline, sizeof(sendline), "The minimum number of players (%d) has been reached. Do you want to start the game now? (yes/no): ", MIN_START_PLAYERS);
                            Writen(rooms[room_index].connfd[0], sendline, strlen(sendline));

                            // 這裡可以重複你原本的判斷邏輯（如設置超時等待房主回應）
                        }

                        return;
                    } else {
                        // 房間不存在或無法加入
                        snprintf(sendline, sizeof(sendline), "Room %d does not exist or is full. You will be randomly assigned to a room.\n", selected_room_id);
                        Writen(connfd[total_id], sendline, strlen(sendline));
                    }
                }
                assignToRoom(connfd, name, listenfd);
            }

        }
    }
}