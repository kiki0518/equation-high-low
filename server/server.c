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
// unpv13e/lib/unp.h

void sig_chld(int signo){
    pid_t pid;
    int   stat;
    while((pid = waitpid(-1, &stat, WNOHANG)) > 0);
    // -1: wait for the first terminated child
	// WNOHANG: not block if there are no more terminated children
    return;
}

void letPlay(int id[], char name[][], int connfd[]){
    
}

int main(){
    int listenfd, maxfdp;
    int n, total_id, notPair_id, connfd[100];
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
    notPair_id = total_id = 0;
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
            }
        }
    }
}