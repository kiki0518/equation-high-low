#include "unp.h"

void sig_chld(int signo) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
        printf("child %d terminated\n", pid);
    return;
}

int main(int argc, char **argv) {
    int n, x, myPort, tcplistenfd, tcpconnfd, udpsockfd;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in cliaddr, tcpservaddr, tcpcliaddr, udpservaddr, udpcliaddr, myaddr;
    char myIP[16], sendline[MAXLINE], recvline[MAXLINE + 1], buffer[100], udpcli[100];
    char student_id[10] = "112550015";

    // TCP server setup
    tcplistenfd = Socket(AF_INET, SOCK_STREAM, 0);
    bzero(&tcpservaddr, sizeof(tcpservaddr));
    tcpservaddr.sin_family = AF_INET;
    tcpservaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcpservaddr.sin_port = htons(SERV_PORT + 3);
    Bind(tcplistenfd, (SA *) &tcpservaddr, sizeof(tcpservaddr));
    Listen(tcplistenfd, LISTENQ);
    Signal(SIGCHLD, sig_chld);  /* must call waitpid() */

    // UDP client setup
    bzero(&udpcliaddr, sizeof(udpcliaddr));
    udpcliaddr.sin_family = AF_INET;
    udpcliaddr.sin_port = htons(SERV_PORT + 3);
    Inet_pton(AF_INET, argv[2], &udpservaddr.sin_addr);
    udpsockfd = Socket(AF_INET, SOCK_DGRAM, 0);
    connect(udpsockfd, (struct sockaddr *) &udpservaddr, sizeof(udpservaddr));

    // Get local IP address and port number using getsockname
    bzero(&myaddr, sizeof(myaddr));
    socklen_t len = sizeof(myaddr);
    getsockname(tcplistenfd, (struct sockaddr *)&myaddr, &len);
    Inet_ntop(AF_INET, &myaddr.sin_addr, myIP, sizeof(myIP));
    myPort = ntohs(myaddr.sin_port); 

    // Send UDP message to teacher
    snprintf(sendline, sizeof(sendline), "%d %s %s", 11, student_id, myIP);
    Write(udpsockfd, sendline, strlen(sendline));
    printf("Sent: %s\n", sendline);

    // Receive UDP response (n value)
    n = Read(udpsockfd, recvline, MAXLINE);
    recvline[n] = 0;
    printf("Received: %s\n", recvline);
    sscanf(recvline, "%d %s %s", &n, buffer, udpcli);
    
    // Handle error responses
    if (n == 99) {
        printf("Not the right IP address ERROR\n");
        return 0;
    } else if (n == 98) {
        printf("Code Internal ERROR\n");
        return 0;
    }

    // Send UDP message with code 13
    snprintf(sendline, sizeof(sendline), "%d %s %d", 13, student_id, myPort);
    Writen(udpsockfd, sendline, strlen(sendline));
    printf("Sent: %s\n", sendline);

    // Handle TCP client connections
    for (int t = n; t > 0; t--) {
        clilen = sizeof(cliaddr);
        if ((tcpconnfd = accept(tcplistenfd, (SA *) &cliaddr, &clilen)) < 0) {
            if (errno == EINTR)
                continue; // Back to the for loop
            else
                err_sys("accept error");
        }
        
        // Create child process to handle TCP connection
        if ((childpid = Fork()) == 0) {  /* child process */
            Close(tcplistenfd);  /* Close listening socket */
            
            // Read the message from TCP client
            n = Read(tcpconnfd, recvline, MAXLINE);
            recvline[n] = 0;
            sscanf(recvline, "%d\n", &x);
            printf("Received: %s\n", recvline);

            // Get peer address and port number
            socklen_t len = sizeof(tcpcliaddr);
            getpeername(tcpconnfd, (struct sockaddr*)&tcpcliaddr, &len);
            printf("Client IP address: %s\n", inet_ntoa(tcpcliaddr.sin_addr));
            snprintf(sendline, sizeof(sendline), "%d %d\n", ntohs(tcpcliaddr.sin_port), x);
            Writen(tcpconnfd, sendline, strlen(sendline));
            printf("Sent: %s", sendline);

            // Receive response from TCP client
            n = Read(tcpconnfd, recvline, MAXLINE);
            recvline[n] = 0;
            printf("Received: %s\n", recvline);
            if (strcmp(recvline, "ok\n") == 0) {
                printf("Successfully handled, remaining %d times\n", t - 1);
            } else if (strcmp(recvline, "nak\n") == 0) {
                printf("TCP client returned ERROR!\n");
                return 0;
            }
            exit(0);  // Exit the child process
        }
        
        Close(tcpconnfd);  /* Parent closes connected socket */
    }

    // Receive final UDP message to check if everything was successful
    n = Recvfrom(udpsockfd, recvline, MAXLINE, 0, NULL, NULL);
    recvline[n] = 0;
    printf("Received: %s\n", recvline);
    if (strcmp(recvline, "ok") == 0) {
        printf("Successfully completed!\n");
    } else if (strcmp(recvline, "nak") == 0) {
        printf("UDP returned ERROR!\n");
    }

    return 0;
}
