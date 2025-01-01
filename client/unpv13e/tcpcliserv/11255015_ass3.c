#include	"unp.h"
#include "string.h"
#include "sys/socket.h"
#include "stdbool.h"

char* concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}


void str_cli(FILE *fp, int sockfd)
{
	char sendline[MAXLINE], recvline[MAXLINE];
	char student_id[11] = "112550015 ";
	struct sockaddr_in clientAddr;
	int clientAddrLen = sizeof(clientAddr);
    char hostname[MAXLINE];
	int n;

	getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen);
    char* s = concat(student_id, inet_ntoa(clientAddr.sin_addr));
    s = concat(s, "\n");
	Writen(sockfd, s, strlen(s));
	printf("sent: %s\n", s);
	
	if ((n = Read(sockfd, hostname, MAXLINE)) == 0)
		err_quit("str_cli: server terminated prematurely");
	hostname[n] = 0;
	printf("recv: %s\n", hostname);

	struct addrinfo hints, *res;
	struct in_addr addr;
	int err;

	memset(&hints, 0, sizeof(hints));
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;

	if((err = getaddrinfo(hostname, NULL, &hints, &res)) != 0) {
		printf("error %d : %s\n", err, gai_strerror(err));
		return;
	}
	addr.s_addr = ((struct sockaddr_in*)(res->ai_addr))->sin_addr.s_addr;
	Writen(sockfd, inet_ntoa(addr), strlen(inet_ntoa(addr)));
	printf("sent: %s\n", inet_ntoa(addr));

	if ((n = Read(sockfd, recvline, MAXLINE)) == 0)
		err_quit("str_cli: server terminated prematurely");
	recvline[n] = 0;
	printf("recv: %s\n", recvline);

	if(strcmp(recvline, "bad") == 0) 	return;
	
	Writen(sockfd, s, strlen(s));
	printf("sent: %s\n", s);

	if ((n = Read(sockfd, recvline, MAXLINE)) == 0)
		err_quit("str_cli: server terminated prematurely");
	recvline[n] = 0;
	printf("recv: %s\n", recvline);
	if(strcmp(recvline, "ok") == 0) {
		printf("successfully\n");
		return;
	}
	return;
}

int main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_in	servaddr;

	if (argc != 2)
		err_quit("usage: tcpcli <IPaddress>");

	sockfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT+2);
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);

	Connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	str_cli(stdin, sockfd);		/* do it all */

	exit(0);
}
