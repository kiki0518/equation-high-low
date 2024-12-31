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
#define MAX_SPECS 100
#define MAX_ROOMS 10
#define ROOM_CAPACITY 5
#define MIN_START_PLAYERS 3
// unpv13e/lib/unp.h

typedef struct {
    int player_count;
    int spec_count;
    int connfd[MAX_PLAYERS];
    int spec_connfd[MAX_SPECS];
    char name[MAX_PLAYERS][100];
    char spec_name[MAX_SPECS][100];
    int room_id;
    int host_id; // The player ID of the room host
    int close;
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
    char arguments[MAX_PLAYERS+MAX_SPECS+2][300];
    char *args[MAX_PLAYERS+MAX_SPECS+5];
    strcpy(arguments[0],"./game");
    args[0] = arguments[0];
    char buffer[12]; // 足够存储一个32位整数的字符串表示
    sprintf(buffer, "%d", room->player_count);
    args[1] = buffer;
    
    for(int i=0; i<room->player_count; i++){
        sprintf(arguments[i+1],"%d",room->connfd[i]);
        args[i+2] = arguments[i+1];
    }
    for(int i=0; i<room->spec_count; i++){
        sprintf(arguments[i+1],"%d",room->spec_connfd[i]);
        args[room->player_count+2+i] = arguments[i+1];
    }
    
    execv("./game",args);

    sleep(5); // Simulate game duration
    snprintf(sendline, sizeof(sendline), "Game in Room %d ends.\n", room->room_id);
    for (int i = 0; i < room->player_count; ++i) {
        Writen(room->connfd[i], sendline, strlen(sendline));
        Close(room->connfd[i]);
    }
    room->player_count = 0; // Reset the room
    room->spec_count = 0;
}

int checkRoomAvailability(int room_id) {
    for (int i = 0; i < room_count; i++) {
        if (rooms[i].room_id == room_id && rooms[i].player_count < ROOM_CAPACITY && rooms[i].close == 0) {
            return i; // 返回房間的索引
        }
    }
    return -1; // 沒有找到符合條件的房間
}

void set_timeout(Room* room, int listenfd){
// 設置超時等待房主回應
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(room->connfd[0], &readfds);
    struct timeval timeout = {30, 0};  // 30秒超時

    int activity = select(room->connfd[0] + 1, &readfds, NULL, NULL, &timeout);
    if (activity > 0 && FD_ISSET(room->connfd[0], &readfds)) {
        // 讀取房主的回應
        char response[100];
        int n = Read(room->connfd[0], response, sizeof(response) - 1);
        response[n] = '\0';

        if (strcasecmp(response, "yes") == 0){
            // 房主選擇開始遊戲
            printf("Room %d: The host has started the game.\n", room->room_id);
            room->close = 1;
            pid_t pid = fork();
            if (pid == 0) {
                Close(listenfd);
                letPlay(room, listenfd);
                exit(0);
            }
        } 
        else{
            // 房主選擇不開始，繼續等待其他玩家加入
            printf("Room %d: The host chose not to start the game yet.\n", room->room_id);
        }
    } 
    else{
        // 超時未回應，繼續等待玩家加入
        printf("Room %d: The host did not respond in time. Waiting for more players...\n", room->room_id);
    }
}

int assignAsSpectator(int connfd, char* name, int listenfd) {
    char sendline[MAXLINE];

    snprintf(sendline, sizeof(sendline), "Please enter the room ID you'd like to spectate: ");
    Writen(connfd, sendline, strlen(sendline));
    char input[100];
    int n = Read(connfd, input, sizeof(input) - 1);
    input[n] = '\0';

    int selected_room_id = atoi(input);

    for (int i = 0; i < room_count; i++) {
        if (rooms[i].room_id == selected_room_id && rooms[i].close == 0) {
            int spectator_id = rooms[i].spec_count;
            rooms[i].spec_connfd[spectator_id] = connfd;
            strcpy(rooms[i].spec_name[spectator_id], name);
            rooms[i].spec_count++;

            snprintf(sendline, sizeof(sendline), "You are now spectating Room %d.\n", rooms[i].room_id);
            Writen(connfd, sendline, strlen(sendline));
            return rooms[i].room_id;
        }
    }
    snprintf(sendline, sizeof(sendline), "Room %d does not exist or the room has close.\n", selected_room_id);
    Writen(connfd, sendline, strlen(sendline));
    return -1;
}

int assignToSpecificRoom(int connfd, char* player_name, int listenfd) {
    char sendline[MAXLINE];
    char input[100];

    // 提示玩家输入房间ID
    snprintf(sendline, sizeof(sendline), "Please enter the room ID you'd like to join: ");
    Writen(connfd, sendline, strlen(sendline));
    int n = Read(connfd, input, sizeof(input) - 1);
    input[n] = '\0';

    int selected_room_id = atoi(input); // 转换输入为房间ID
    int room_index = checkRoomAvailability(selected_room_id);

    if (room_index != -1){
        // 如果房间存在且有空位
        int player_id = rooms[room_index].player_count;
        rooms[room_index].connfd[player_id] = connfd;
        strcpy(rooms[room_index].name[player_id], player_name);
        rooms[room_index].player_count++;

        snprintf(sendline, sizeof(sendline), "You are added to Room %d as player #%d.\n", rooms[room_index].room_id, player_id + 1);
        Writen(connfd, sendline, strlen(sendline));

        // 如果玩家数量达到起始条件，通知房主是否开始游戏
        if (rooms[room_index].player_count >= MIN_START_PLAYERS) {
            snprintf(sendline, sizeof(sendline), "The minimum number of players (%d) has been reached. Do you want to start the game now? (yes/no): ", MIN_START_PLAYERS);
            Writen(rooms[room_index].connfd[0], sendline, strlen(sendline));
            set_timeout(&rooms[room_index], listenfd);
        }

        return 1; // 表示成功加入特定房间
    } 
    else{
        // 房间不存在或已满
        snprintf(sendline, sizeof(sendline), "Room %d does not exist or is full. You can input correct room_id or choose to be randomly assigned to a room.\n", selected_room_id);
        Writen(connfd, sendline, strlen(sendline));
        return 0; // 表示未成功加入
    }
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
            if (rooms[i].player_count >= MIN_START_PLAYERS){
                snprintf(sendline, sizeof(sendline), "The minimum number of players (%d) has been reached. Do you want to start the game now? (yes/no): ", MIN_START_PLAYERS);
                Writen(rooms[i].connfd[0], sendline, strlen(sendline));
                set_timeout(&rooms[i], listenfd);
            } 
            else if(rooms[i].player_count == ROOM_CAPACITY) {
                rooms[i].close = 1;
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
        new_room->spec_count = 0;
        new_room->connfd[0] = connfd;
        strcpy(new_room->name[0], name);
        new_room->host_id = 0; // First player becomes the host
        new_room->close = 0;

        char sendline[MAXLINE];
        snprintf(sendline, sizeof(sendline), "You are the host of Room %d.\n", new_room->room_id);
        Writen(connfd, sendline, strlen(sendline));
        sleep(2);

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
    char name[100][100], sendline[MAXLINE], recvline[MAXLINE];
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    fd_set rset, recSet;

    // create listen socket
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    // port number set?
    servaddr.sin_port = htons(SERV_PORT);

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
                    name[total_id][n-1] = '\0';
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
                
                snprintf(sendline, sizeof(sendline), "You are the #%d user. You may now play with computer or wait for other users.\n", total_id);
                Writen(connfd[total_id], sendline, strlen(sendline));
                printf("Send: %s is the #%d user.\n", name[total_id], total_id);

                sleep(1);
                snprintf(sendline, sizeof(sendline), "You can go next.");
                Writen(connfd[total_id], sendline, strlen(sendline));


                n = Read(connfd[total_id], recvline, MAXLINE);
                recvline[n] = '\0';
                // Assign player to a room
                char input[100], input1[100];
                if(strcmp(recvline, "can continue") == 0){
                    while(1){
                        snprintf(sendline, sizeof(sendline), "Do you want to join as a player or a spectator? (player/spectator): ");
                        Writen(connfd[total_id], sendline, strlen(sendline));
                        int n = Read(connfd[total_id], input1, sizeof(input1) - 1);
                        input1[n-1] = '\0';

                        if(strcasecmp(input1, "spectator") != 0 && strcasecmp(input1, "player") != 0){
                            snprintf(sendline, sizeof(sendline), "Invalid input. Please input again.\n");
                            Writen(connfd[total_id], sendline, strlen(sendline));
                            sleep(2);
                        }
                        else{
                            snprintf(sendline, sizeof(sendline), "You can end this and go next.");
                            Writen(connfd[total_id], sendline, strlen(sendline));
                            break;
                        }
                    }
                }

                n = Read(connfd[total_id], recvline, MAXLINE);
                recvline[n] = '\0';
                printf("Recv: %s\n", recvline);
                if(strcmp(recvline, "I get.") == 0){
                    // to be a spectator
                    if(strcasecmp(input1, "spectator") == 0){
                        while(1){
                            // room_id start from 1
                            if(assignAsSpectator(connfd[total_id], name[total_id], listenfd) > 0){
                                break;
                            }
                        }
                    }
                    // to be a player
                    else{
                        // 問玩家是否要選擇特定房間
                        while(1){
                            snprintf(sendline, sizeof(sendline), "Do you want to join a specific room? (yes/no): ");
                            Writen(connfd[total_id], sendline, strlen(sendline));
                            n = Read(connfd[total_id], input, sizeof(input) - 1);
                            input[n-1] = '\0';
                            printf("Recv: %s\n", input);

                            if (strcasecmp(input, "yes") == 0){
                                // 如果玩家想加入特定房间，调用函数处理
                                if(assignToSpecificRoom(connfd[total_id], name[total_id], listenfd)){
                                    snprintf(sendline, sizeof(sendline), "Yes Success!\n");
                                    Writen(connfd[total_id], sendline, strlen(sendline));
                                    sleep(2);
                                    break;
                                }
                                
                            }
                            else if(strcasecmp(input, "no") == 0){
                                // 如果玩家不想加入特定房间，直接随机分配
                                if(assignToRoom(connfd[total_id], name[total_id], listenfd)){
                                    snprintf(sendline, sizeof(sendline), "No Success!\n");
                                    Writen(connfd[total_id], sendline, strlen(sendline));
                                    sleep(2);
                                    break;
                                }
                                
                            }
                        }
                    }
                }
                
            }
        }
    }
}