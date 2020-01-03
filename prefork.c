#include "csapp.h"
// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <stdio.h>
// #include <resolv.h>
// #include <errno.h>

#define MY_PORT         9999
#define PREFORK_PROC_N  10


void sigchild_handler(int sig){
    while (waitpid(-1, 0, WNOHANG) > 0);
    return;
}


void run_server(int sockfd, struct sockaddr_in client_addr, int addrlen, char buffer[]){
    int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
    printf("%s:%d connected in fd %d\n",
        inet_ntoa(client_addr.sin_addr),
        ntohs(client_addr.sin_port),
        clientfd);
    while (1){
        size_t size;
        size = recv(clientfd, buffer, MAXBUF, 0);
        printf("Received %s", buffer);
        if (buffer[0] == 'q') {
            close(clientfd);
            exit(0);
        }
        send(clientfd, buffer, size, 0);
        memset(buffer, 0, MAXBUF);
    }
}


int main(int argc, char const *argv[]){
    int sockfd;
	struct sockaddr_in self;
	char buffer[MAXBUF];
    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
    {
        perror("Socket");
        exit(errno);
    }

	/*---Initialize address/port structure---*/
	bzero(&self, sizeof(self));
	self.sin_family = AF_INET;
	self.sin_port = htons(MY_PORT);
	self.sin_addr.s_addr = INADDR_ANY;

	/*---Assign a port number to the socket---*/
    if ( bind(sockfd, (struct sockaddr*)&self, sizeof(self)) != 0 )
	{
		perror("socket--bind");
		exit(errno);
	}
    if (listen(sockfd, 20) != 0){
        perror("socket--listen");
        exit(errno);
    }
    // Signal(SIGCHLD, sigchild_handler);
    int i;
    int clientfd;
    struct sockaddr_in client_addr;
    int addrlen=sizeof(client_addr);
    for (i=1; i<PREFORK_PROC_N; i++){
        if (Fork()==0){
            run_server(sockfd, client_addr, addrlen, buffer);
        }
    }
    while (1){
        int ch_pid = wait(NULL);
        if (Fork() == 0){
            run_server(sockfd, client_addr, addrlen, buffer);
        }
    }
    close(sockfd);
    return 0;
}