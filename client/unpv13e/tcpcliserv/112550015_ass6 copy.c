#include	"unp.h"
#include	<stdlib.h>
#include	<stdio.h>
#include    <string.h>

/*
The server needs to perform the following tasks:

Accept client connections to establish a chatroom that supports up to 10 users.

Notify users of new members joining:

When a new user joins, they will receive a message from the server.
All existing users will also receive a message from the server informing them of the new user’s entry.
Relay messages in the chatroom:

The server will prefix messages with the sender’s ID before forwarding them to all other users (e.g., as shown in the diagram with (Alice) and (Bob) prefixes).
The order of message transmission from participants is not fixed (e.g., in the diagram, Bob sends two consecutive messages), and the server must handle this appropriately.
Handle users leaving the chatroom via Ctrl+D:

Pressing Ctrl+D indicates the user is leaving the chatroom properly.
The server must respond to the user leaving with a specific message (e.g., "Bye!\n").
The server must also inform other chatroom members that this user has left.
Handle abrupt disconnections via Ctrl+C:

If a user abruptly ends their session by pressing Ctrl+C, the server will not respond to the disconnected user (since the process has already ended).
However, the server must notify the remaining users that this member has left.
Accept new members after disconnections:

After a user leaves (whether properly via Ctrl+D or abruptly via Ctrl+C), as long as the total number of users does not exceed 10, the server should still be able to accept new members joining the chatroom.
*/

void sig_chld(int signo) {
    pid_t   pid;
    int     stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);
    return;
}

int main(int argc, char **argv) {
	int					listenfd, connfd[10], n, maxfdp1, flag[10], breakFlag, clicnt, currcnt;
	pid_t				childpid;
	socklen_t			clilen[10];
	struct sockaddr_in	servaddr, cliaddr[10];
    void				sig_chld(int);
    char                name[10][20], message[10][100], sendline[MAXLINE], recvline[MAXLINE], blank[MAXLINE], buff[100];
    fd_set rset;

    // TCP server setup
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT + 5);
	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
	Listen(listenfd, LISTENQ);
	Signal(SIGCHLD, sig_chld);

    while(1) {
        clicnt = 0;
        // Connect clients
        for(int i = 0; i < 10; i++){
            clilen[i] = sizeof(cliaddr[i]);
            if ((connfd[i] = accept(listenfd, (SA *) &cliaddr[i], &clilen[i])) < 0) {
                if (errno == EINTR) continue; // try again
                else err_sys("accept error");
            }
            printf("Client%d connected.\n", i + 1);
            n = Read(connfd[i], name[i], 20);
            name[i][n] = '\0';
            printf("Recv: %s\n", name[i]);
            snprintf(message[i], sizeof(message[i]), "You are the #%d user.\n");
            Writen(connfd[i], message[i], strlen(message[i]));
            printf("Sent: %s is the #%d user.\n", name[i], i);
            getpeername(connfd[i], (SA *) &cliaddr[i], &clilen[i]);
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


        
        }

        
        // start chatting
        if ( (childpid = Fork()) == 0) {	/* child process */
            Close(listenfd);	/* close listening socket */
            // flag[0] = flag[1] = 0;
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
        
    for(int i = 0; i < clicnt; i++){
        Close(connfd[i]);
    }
    printf("Server terminated.\n");
    return 0;
}

