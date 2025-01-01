#include	"unp.h"
#include	<stdlib.h>
#include	<stdio.h>
#include    <string.h>

void sig_chld(int signo) {
    pid_t   pid;
    int     stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);
    return;
}

int main(int argc, char **argv) {
	int					listenfd, connfd[2], n, maxfdp1, flag[2], breakFlag;
	pid_t				childpid;
	socklen_t			clilen[2];
	struct sockaddr_in	servaddr, cliaddr[2];
    void				sig_chld(int);
    char                name[2][20], message[2][100], sendline[MAXLINE], recvline[MAXLINE], blank[MAXLINE], buff[100];
    fd_set rset;

    // TCP server setup
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT + 4);
	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
	Listen(listenfd, LISTENQ);
	Signal(SIGCHLD, sig_chld);

    while(1) {
        // Connect 2 clients
        for(int clicnt = 0; clicnt < 2; clicnt++){
            clilen[clicnt] = sizeof(cliaddr[clicnt]);
            if ((connfd[clicnt] = accept(listenfd, (SA *) &cliaddr[clicnt], &clilen[clicnt])) < 0) {
                if (errno == EINTR) continue; // try again
                else err_sys("accept error");
            }
            printf("Client%d connected.\n", clicnt + 1);
            n = Read(connfd[clicnt], name[clicnt], 20);
            name[clicnt][n] = '\0';
            printf("Recv: %s\n", name[clicnt]);
            if(!clicnt) {
                snprintf(message[clicnt], sizeof(message[clicnt]), "You are the 1st user. Wait for the second one!\n");
                Writen(connfd[clicnt], message[clicnt], strlen(message[clicnt]));
                printf("Sent: %s is the 1st user.\n", name[clicnt]);
            } else {
                snprintf(message[clicnt], sizeof(message[clicnt]), "You are the 2nd user.\n");
                Writen(connfd[clicnt], message[clicnt], strlen(message[clicnt]));
                printf("Sent: %s is the 2nd user.\n", name[clicnt]);
            }
            getpeername(connfd[clicnt], (SA *) &cliaddr[clicnt], &clilen[clicnt]);
        }

        // Send peer's info
        snprintf(sendline, sizeof(sendline), "The second user is %s from %s\n", name[1], 
        Inet_ntop(AF_INET, (SA *) &cliaddr[1].sin_addr, buff, sizeof(buff)));
        Writen(connfd[0], sendline, strlen(sendline));
        printf("Sent: %s", sendline);

        snprintf(sendline, sizeof(sendline), "The first user is %s from %s\n", name[0],
        Inet_ntop(AF_INET, (SA *) &cliaddr[0].sin_addr, buff, sizeof(buff)));
        Writen(connfd[1], sendline, strlen(sendline));
        printf("Sent: %s", sendline);

        
        // start chatting
        if ( (childpid = Fork()) == 0) {	/* child process */
            Close(listenfd);	/* close listening socket */
            flag[0] = flag[1] = 0;
            FD_ZERO(&rset);
            while(1) {
                breakFlag = 0;
                FD_SET(connfd[0], &rset);
                FD_SET(connfd[1], &rset);
                maxfdp1 = max(connfd[0], connfd[1]) + 1;
                Select(maxfdp1, &rset, NULL, NULL, NULL);   // block until readable

                for(int i = 0; i < 2; i++){
                    if (FD_ISSET(connfd[i], &rset)) { // client i+1 is readable
                        n = Readline(connfd[i], recvline, MAXLINE);
                        if (n <= 0){
                            printf("Client %d disconnected.\n", i + 1);
                            flag[i] = 1;
                        } else { // readline success, send line to peer
                            snprintf(sendline, sizeof(sendline), "(%s) %s", name[i], recvline);
                            snprintf(blank, sizeof(blank), "(%s) \n", name[i]);
                            if (strcmp(sendline, blank) != 0){
                                Writen(connfd[1 - i], sendline, strlen(sendline));
                                printf("Sent client%d's message to client%d.\n", i + 1, 2 - i);
                            }
                        }
                    }
                }
                for(int i = 0; i < 2; i++){
                    if (flag[i]){
                        snprintf(sendline, sizeof(sendline), "(%s left the room. Press Ctrl + D to leave.)\n", name[i]);
                        Writen(connfd[1 - i], sendline, strlen(sendline));
                        printf("Informed client%d that client%d has disconnected.\n", 1 - i, i);
                        shutdown(connfd[1 - i], SHUT_WR); // send FIN to peer
                        printf("Shutting down client%d's connection.\n", 1 - i);
                        Readn(connfd[1 - i], recvline, MAXLINE);
                        snprintf(sendline, sizeof(sendline), "(%s left the room.)\n", name[1 - i]);
                        Writen(connfd[i], sendline, strlen(sendline));
                        printf("Informed client%d that client%d has disconnected.\n", i, 1 - i);
                        shutdown(connfd[i], SHUT_WR); // send FIN to socket
                        printf("Shutting down client%d's connection.\n", i);
                        Close(connfd[1 - i]);
                        Close(connfd[i]);
                        breakFlag = 1;
                        break;
                    }
                }
                if (breakFlag)  break;
            }
            exit(0);
        }
        Close(connfd[0]);
        Close(connfd[1]);
	}
    printf("Server terminated.\n");
    return 0;
}

