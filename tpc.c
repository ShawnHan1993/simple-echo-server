#include "csapp.h"

#define MY_PORT   9999
#define MAXBUF_THREAD    1024

sem_t sem;
size_t total_size = 0;

void print_total_size(int sig){
    P(&sem);
    printf("Received %d chars in total.\n", (unsigned int)total_size);
    V(&sem);
}


void *thread_routine(int *clientfd){
    int clifd = *clientfd;
    pthread_detach(pthread_self());
    char buffer[MAXBUF_THREAD];
    size_t size;
    while (1){
        size = recv(clifd, buffer, MAXBUF_THREAD, 0);
        P(&sem);
        total_size += size;
        V(&sem);
        printf("Received %s", buffer);
        raise(SIGUSR1);
        if (buffer[0] == 'q') {
            close(clifd);
            return NULL;
        }
        send(clifd, buffer, size, 0);
        memset(buffer, 0, MAXBUF_THREAD);
    }
}


int main(int argc, char const *argv[]){
    Sem_init(&sem, 0, 1);
    int sockfd;
	struct sockaddr_in self;
	// char buffer[MAXBUF];
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
    signal(SIGUSR1, print_total_size);
    while (1){
        struct sockaddr_in client_addr;
        int addrlen=sizeof(client_addr);
        int clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
        printf("%s:%d connected in fd %d\n",
            inet_ntoa(client_addr.sin_addr),
            ntohs(client_addr.sin_port),
            clientfd);
        pthread_t tid;
        pthread_create(&tid, NULL, thread_routine, &clientfd);
    }
    close(sockfd);
    return 0;
}


