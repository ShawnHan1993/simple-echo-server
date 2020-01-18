#include "csapp.h"
#include <sys/poll.h>

#define MY_PORT   9999
#define BUFSIZE   1024
#define MAX_CLIENTS 2048


typedef struct {
    struct pollfd p[MAX_CLIENTS];
    int nfd;
    int nread;
    int sockfdIdx; // To remember which location in p stores the sockfd
    int stdinIdx; // To remember which location in p stores the stdinfd
} client_pool_t;

int add_client(client_pool_t *client_pool, int fd){
    if (client_pool->nfd >= MAX_CLIENTS) {app_error("Too many clients!");}
    client_pool->p[client_pool->nfd].fd = fd;
    client_pool->p[client_pool->nfd].events = POLLIN;
    client_pool->nfd ++;
    return client_pool->nfd - 1;
}

void init_client_pool(client_pool_t *client_pool, int sockfd){
    client_pool->nfd = 0;
    client_pool->nread = 0;
    int i;
    for (i = 0; i < MAX_CLIENTS; i++){
        client_pool->p[i].fd = -1;
        client_pool->p[i].events = POLLIN;
        client_pool->p[i].revents = 0;
    }
    int stdinIdx = add_client(client_pool, STDIN_FILENO);
    client_pool->stdinIdx = stdinIdx;
    int sockfdIdx = add_client(client_pool, sockfd);
    client_pool->sockfdIdx = sockfdIdx;
    return;
}

void rm_client(client_pool_t *client_pool, int fd){
    int i, stopIdx;
    for (i = client_pool->nfd - 1; i >= 0; i --){
        if (client_pool->p[i].fd == fd){
            stopIdx = i;
        }
    }
    for (i = stopIdx; i < client_pool->nfd - 1; i ++){
        client_pool->p[i] = client_pool->p[i + 1];
    }
    client_pool->nfd --;
}

void check_ready_fd(client_pool_t *client_pool){
    int i, fd;
    char buffer[BUFSIZE];
    for (i = 0; (i < client_pool->nfd && client_pool->nread > 0); i++){
        if (client_pool->p[i].revents & POLLIN){
            client_pool->p[i].revents = 0;
            client_pool->nread --;
            size_t size;
            size = recv(client_pool->p[i].fd, buffer, BUFSIZE, 0);
            printf("Received %s", buffer);
            if (buffer[0] == 'q') {
                close(client_pool->p[i].fd);
                rm_client(client_pool, client_pool->p[i].fd);
            }
            send(client_pool->p[i].fd, buffer, size, 0);
            memset(buffer, 0, BUFSIZE);
        }
    }
}

void broadcast(client_pool_t *client_pool, char *cmd, int cmd_len){
    int i;
    for (i = 0; i < client_pool->nfd; i++)
        send(client_pool->p[i].fd, cmd, cmd_len, 0);
}

int main(int argc, char const *argv[]){
    int sockfd;
	struct sockaddr_in self;
	char buffer[BUFSIZE];
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
    int clientfd;
    struct sockaddr_in client_addr;
    int addrlen=sizeof(client_addr);

    client_pool_t client_pool;
    init_client_pool(&client_pool, sockfd);
    char cmd_buf[BUFSIZE];
    while (1){
        client_pool.nread = poll(client_pool.p, client_pool.nfd, NULL);
        if (client_pool.p[client_pool.sockfdIdx].revents & POLLIN){
            clientfd = Accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
            printf("%s:%d connected in fd %d\n",
                inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port),
                clientfd);
            add_client(&client_pool, clientfd);
        }
        else if (client_pool.p[client_pool.stdinIdx].revents & POLLIN){
            size_t size;
            size = read(STDIN_FILENO, cmd_buf, BUFSIZE);
            printf("Received cmd from stdin: %s", cmd_buf);
            if (cmd_buf[0] == 'q'){
                printf("Stop server.");
                break;
            }
            else{
                broadcast(&client_pool, cmd_buf, size);
            }
            memset(cmd_buf, 0, BUFSIZE);
        }
        else {
            // We dont know which fd is ready, so we have to iterate all fds.
            check_ready_fd(&client_pool);
        }
    }
    close(sockfd);
    return 0;
}