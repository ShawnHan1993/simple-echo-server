#include "csapp.h"

#define MY_PORT   9999
#define BUFSIZE    1024


typedef struct {
    int active_fd[FD_SETSIZE];
    int nread;
    fd_set read_set;
    fd_set ready_set;
    int maxfd;
} client_pool_t;


void init_client_pool(client_pool_t *client_pool, int sockfd){
    FD_ZERO(&client_pool->read_set);
    FD_SET(STDIN_FILENO, &client_pool->read_set);
    FD_SET(sockfd, &client_pool->read_set);
    int i;
    for (i = 0; i < FD_SETSIZE; i++){
        client_pool->active_fd[i] = -1;
    }
    client_pool->maxfd = sockfd;
    return;
}

void rm_client(client_pool_t *client_pool, size_t fd){
    if (client_pool->active_fd[fd] != 1){app_error("Closing un-opened client!");}
    if (client_pool->maxfd == fd){
        int i;
        for (i = client_pool->maxfd - 1; i >= 0; i--){
            if (client_pool->active_fd[i] > 0){
                client_pool->maxfd = i;
                break;
            }
        }
    }
    FD_CLR(fd, &client_pool->read_set);
}

void check_ready_fd(client_pool_t *client_pool){
    int i, fd;
    char buffer[BUFSIZE];
    for (i = 0; (i <= client_pool->maxfd) && (client_pool->nread > 0); i++){
        if (FD_ISSET(i, &client_pool->ready_set)){
            client_pool->nread --;
            size_t size;
            size = recv(i, buffer, BUFSIZE, 0);
            printf("Received %s", buffer);
            if (buffer[0] == 'q') {
                close(i);
                rm_client(client_pool, i);
            }
            send(i, buffer, size, 0);
            memset(buffer, 0, BUFSIZE);
        }
    }
}

void broadcast(client_pool_t *client_pool, char *cmd, int cmd_len){
    int i;
    for (i = 0; i <= client_pool->maxfd; i++){
        if (client_pool->active_fd[i] > 0){
            send(i, cmd, cmd_len, 0);
        }
    }
}

void add_client(client_pool_t *client_pool, int fd){
    if (fd > FD_SETSIZE) {app_error("Too many clients!");}
    client_pool->active_fd[fd] = 1;
    if (fd > client_pool->maxfd){
        client_pool->maxfd = fd;
    }
    FD_SET(fd, &client_pool->read_set);
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
        client_pool.ready_set = client_pool.read_set;
        client_pool.nread = Select(client_pool.maxfd + 1, &client_pool.ready_set, NULL, NULL, NULL);
        if (FD_ISSET(sockfd, &client_pool.ready_set)){
            clientfd = Accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
            printf("%s:%d connected in fd %d\n",
                inet_ntoa(client_addr.sin_addr),
                ntohs(client_addr.sin_port),
                clientfd);
            add_client(&client_pool, clientfd);
        }
        else if (FD_ISSET(STDIN_FILENO, &client_pool.ready_set)){
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