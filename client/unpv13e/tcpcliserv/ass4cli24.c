#include "unp.h"

void sig_chld(int signo) {
    pid_t pid;
    int stat;
    while ((pid = waitpid(-1, &stat, WNOHANG)) > 0);
    return;
}

int main(int argc, char **argv) {
    int n, x, myPort, tcplistenfd, tcpconnfd, udpsockfd;
    pid_t childpid;
    socklen_t clilen;
    struct sockaddr_in tcpservaddr, tcpcliaddr, udpservaddr, myaddr;
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
    udpsockfd = Socket(AF_INET, SOCK_DGRAM, 0);
    bzero(&udpservaddr, sizeof(udpservaddr));
    udpservaddr.sin_family = AF_INET;
    udpservaddr.sin_port = htons(SERV_PORT + 3);
    Inet_pton(AF_INET, argv[1], &udpservaddr.sin_addr);
    connect(udpsockfd, (struct sockaddr *) &udpservaddr, sizeof(udpservaddr));

    // Get local IP address and port number using getsockname
    bzero(&myaddr, sizeof(myaddr));
    socklen_t len = sizeof(myaddr);
    getsockname(udpsockfd, (struct sockaddr *)&myaddr, &len);
    Inet_ntop(AF_INET, &myaddr.sin_addr, myIP, sizeof(myIP));
    // socklen_t len = sizeof(udpservaddr);
    // getsockname(udpsockfd, (struct sockaddr *)&udpservaddr, &len);  //fail
    // Inet_ntop(AF_INET, &udpservaddr.sin_addr, myIP, sizeof(myIP));
    // myPort = ntohs(myaddr.sin_port); 
    // printf("myaddr.sin_port: %d\n", myaddr.sin_port);
    // printf("udpservaddr.sin_port:%d\n", udpservaddr.sin_port);
    myPort = ntohs(tcpservaddr.sin_port);   // success

    // Send UDP message to teacher
    snprintf(sendline, sizeof(sendline), "%d %s %s", 11, student_id, myIP);
    send(udpsockfd, sendline, strlen(sendline), 0);
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
    send(udpsockfd, sendline, strlen(sendline), 0);
    printf("Sent: %s\n", sendline);


    // Handle TCP client connections
    for (int t = n; t > 0; t--) {
        clilen = sizeof(tcpcliaddr);
        if ((tcpconnfd = accept(tcplistenfd, (SA *) &tcpcliaddr, &clilen)) < 0) {
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
            printf("Received: %s", recvline);

            // Get peer address and port number
            socklen_t len = sizeof(tcpcliaddr);
            getpeername(tcpconnfd, (struct sockaddr*)&tcpcliaddr, &len);
            // printf("Client IP address: %s\n", inet_ntoa(tcpcliaddr.sin_addr));
            snprintf(sendline, sizeof(sendline), "%d %d\n", ntohs(tcpcliaddr.sin_port), x);
            Writen(tcpconnfd, sendline, strlen(sendline));
            printf("Sent: %s", sendline);

            // Receive response from TCP client
            n = Read(tcpconnfd, recvline, MAXLINE);
            recvline[n] = 0;
            printf("Received: %s", recvline);
            if (strcmp(recvline, "ok\n") == 0) {
                printf("Successfully handled\n=============\n");
            } else if (strcmp(recvline, "nak\n") == 0) {
                printf("TCP client returned ERROR!\n");
                return 0;
            }
            exit(0);  // Exit the child process
        }
        
        Close(tcpconnfd);  /* Parent closes connected socket */
    }

    // Receive final UDP message to check if everything was successful
    // n = Recvfrom(udpsockfd, recvline, MAXLINE, 0, NULL, NULL);
    n = Recv(udpsockfd, recvline, MAXLINE, 0);
    recvline[n] = 0;
    printf("Received: %s\n", recvline);
    if (strcmp(recvline, "ok") == 0) {
        printf("Successfully completed!\n");
    } else if (strcmp(recvline, "nak") == 0) {
        printf("UDP returned ERROR!\n");
    }
    return 0;
}

    
    // Handle TCP client connections
// for (int t = n; t > 0; t--) {
//     clilen = sizeof(tcpcliaddr);
//     printf("Waiting for a TCP connection...\n");
//     if ((tcpconnfd = accept(tcplistenfd, (SA *) &tcpcliaddr, &clilen)) < 0) {
//         if (errno == EINTR) {
//             printf("Interrupted by signal, continue accepting...\n");
//             continue; // Back to the for loop
//         } else {
//             perror("accept error");
//             exit(1);
//         }
//     }
//     printf("Accepted a TCP connection from %s:%d\n",
//            inet_ntoa(tcpcliaddr.sin_addr), ntohs(tcpcliaddr.sin_port));

//     // Create child process to handle TCP connection
//     if ((childpid = Fork()) == 0) {  /* child process */
//         printf("Child process %d started to handle the connection\n", getpid());
//         Close(tcplistenfd);  /* Close listening socket in child */
        
//         // Read the message from TCP client
//         n = Read(tcpconnfd, recvline, MAXLINE);
//         if (n < 0) {
//             perror("Read error");
//             exit(1);
//         } else if (n == 0) {
//             printf("Client closed the connection\n");
//             Close(tcpconnfd);
//             exit(0);
//         }
//         recvline[n] = 0;
//         printf("Received from TCP client: %s\n", recvline);
//         sscanf(recvline, "%d", &x);
//         printf("Parsed x value: %d\n", x);

//         // Get peer address and port number
//         socklen_t len = sizeof(tcpcliaddr);
//         if (getpeername(tcpconnfd, (struct sockaddr*)&tcpcliaddr, &len) < 0) {
//             perror("getpeername error");
//             exit(1);
//         }
//         char client_ip[INET_ADDRSTRLEN];
//         Inet_ntop(AF_INET, &tcpcliaddr.sin_addr, client_ip, sizeof(client_ip));
//         int client_port = ntohs(tcpcliaddr.sin_port);
//         printf("Client IP address: %s\n", client_ip);
//         printf("Client port number: %d\n", client_port);

//         // Prepare and send the response
//         snprintf(sendline, sizeof(sendline), "%d %d\n", client_port, x);
//         printf("Sending to TCP client: %s", sendline);
//         Writen(tcpconnfd, sendline, strlen(sendline));

//         // Receive response from TCP client
//         n = Read(tcpconnfd, recvline, MAXLINE);
//         if (n < 0) {
//             perror("Read error");
//             exit(1);
//         } else if (n == 0) {
//             printf("Client closed the connection after response\n");
//             Close(tcpconnfd);
//             exit(0);
//         }
//         recvline[n] = 0;
//         printf("Received from TCP client: %s\n", recvline);
//         if (strcmp(recvline, "ok\n") == 0) {
//             printf("TCP client acknowledged with 'ok'\n");
//         } else if (strcmp(recvline, "nak\n") == 0) {
//             printf("TCP client responded with 'nak'\n");
//             Close(tcpconnfd);
//             exit(1);
//         } else {
//             printf("Unexpected response from TCP client: %s\n", recvline);
//             Close(tcpconnfd);
//             exit(1);
//         }
//         Close(tcpconnfd);
//         printf("Child process %d finished handling the connection\n", getpid());
//         exit(0);  // Exit the child process
//     }
//     // Parent process continues
//     Close(tcpconnfd);  /* Parent closes connected socket */
//     printf("Parent process closed the connected socket, back to accepting...\n");
// }

