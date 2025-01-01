#include "unp.h"
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

void sig_chld(int signo) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);
    return;
}

int main(int argc, char **argv) {
    int listenfd, n, cur, total, maxfdp1, connfd[15];
    bool online[15] = {false};
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    char names[15][100], recvline[MAXLINE], sendline[MAXLINE];
    fd_set rset, master_set;

    // TCP server setup
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(SERV_PORT + 5);
    Bind(listenfd, (SA *)&servaddr, sizeof(servaddr));
    Listen(listenfd, LISTENQ);
    Signal(SIGCHLD, sig_chld);

    // initialialization
    cur = total = 0;
    FD_ZERO(&master_set);
    FD_SET(listenfd, &master_set);
    maxfdp1 = listenfd + 1;

    while (1) {
        rset = master_set;
        select(maxfdp1, &rset, NULL, NULL, NULL);
        // see if a new client has connected
        if (FD_ISSET(listenfd, &rset)) {
            if (total < 10) {
                for(cur = 0; cur < 10; cur++) {
                    if (!online[cur]) {
                        break;
                    }
                }
                printf("Client #%d connected.\n", cur + 1);
                online[cur] = true;
                clilen = sizeof(cliaddr);
                connfd[cur] = Accept(listenfd, (SA *)&cliaddr, &clilen);
                FD_SET(connfd[cur], &master_set);
                if (connfd[cur] + 1 > maxfdp1)
                    maxfdp1 = connfd[cur] + 1;

                // receive client's name
                n = Read(connfd[cur], names[cur], 100);
                names[cur][n] = '\0';
                printf("Recv: %s\n", names[cur]);
                snprintf(sendline, sizeof(sendline), "You are the #%d user.\n", cur + 1);
                Writen(connfd[cur], sendline, strlen(sendline));

                snprintf(sendline, sizeof(sendline), "You may now type in or wait for other users.\n");
                Writen(connfd[cur], sendline, strlen(sendline));
                printf("Sent: %s is the #%d user.\n", names[cur], cur + 1);
                // broadcast to other clients that a new user has entered
                for (int i = 0; i < 10; i++) {
                    if (online[i]) {
                        snprintf(sendline, sizeof(sendline), "(#%d user %s enters.)\n", cur + 1, names[cur]);
                        Writen(connfd[i], sendline, strlen(sendline));
                    }
                }
                total++;
            }
        }
        // check if there is any client sending message
        for (int i = 0; i < 10; i++) {
            if (!online[i]) continue;
            if (FD_ISSET(connfd[i], &rset)) {   // message from client i
                n = Readline(connfd[i], recvline, MAXLINE);
                if (n <= 0) {   // client i disconnected
                    printf("Client #%d disconnected.\n", i + 1);
                    total--;

                    // send bye message to client i and broadcast to others
                    snprintf(sendline, sizeof(sendline), "Bye!\n");
                    Writen(connfd[i], sendline, strlen(sendline));
                    if (total >= 1) {
                        if(total == 1) {
                            snprintf(sendline, sizeof(sendline), "(%s left the room. You are the last one. Press Ctrl+D to leave or wait for a new user.)\n", names[i]);
                        } else {
                            snprintf(sendline, sizeof(sendline), "(%s left the room. %d users left)\n", names[i], total);
                        }
                        for (int j = 0; j < 10; j++) {
                            if (j != i && online[j]) 
                                Writen(connfd[j], sendline, strlen(sendline));
                        }
                    } else {
                        printf("No one is left in the room.\n");
                    }

                    online[i] = false;
                    FD_CLR(connfd[i], &master_set);
                    shutdown(connfd[i], SHUT_WR);
                    Close(connfd[i]);

                } else if (n == 1) {  // send "\n" to other clients
                    for (int j = 0; j < 10; j++) {
                        if (j != i && online[j]) 
                            Writen(connfd[j], "\n", 1);
                    }
                } else if (n > 1) {  // send to other clients (ignore messages with only "\n")
                    printf("Recv: %s from client%d with connfd = %d\n", recvline, i, connfd[i]);
                    snprintf(sendline, sizeof(sendline), "(%s) %s", names[i], recvline);
                    printf("Sent client%d's message to others.\n", i);
                    for(int j = 0; j < 10; j++) {
                        if (j != i && online[j]) 
                            Writen(connfd[j], sendline, strlen(sendline));
                    }
                }
            }
        }
    }
    return 0;
}