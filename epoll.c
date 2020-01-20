#include "csapp.h"
#include <sys/epoll.h>


#define MY_PORT   9999
#define BUFSIZE   1024
#define EVENT_NUM 20


void init_interest_list(int epfd, int sockfd, struct epoll_event *conn_and_cmd_event){
    conn_and_cmd_event[0].events = EPOLLIN;
    conn_and_cmd_event[0].data.fd = sockfd;
    conn_and_cmd_event[1].events = EPOLLIN;
    conn_and_cmd_event[1].data.fd = STDIN_FILENO;
    epoll_ctl(epfd, EPOLL_CTL_ADD, sockfd, &conn_and_cmd_event[0]);
    epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &conn_and_cmd_event[1]);
}

void rm_client(int epfd, size_t fd){
    // The following event is ignored
    // see http://man7.org/linux/man-pages/man2/epoll_ctl.2.html
    struct epoll_event tmp;
    tmp.events = EPOLLIN | EPOLLRDHUP;
    tmp.data.fd = fd;
    // ----
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, &tmp);
}

void add_client(int epfd, int fd){
    struct epoll_event tmp;
    tmp.events = EPOLLIN | EPOLLRDHUP;
    tmp.data.fd = fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &tmp);
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


    int epfd;
    struct epoll_event conn_and_cmd_event[2];
    struct epoll_event events[EVENT_NUM];
    epfd = epoll_create(1);

    int nevents;

    init_interest_list(epfd, sockfd, conn_and_cmd_event);
    char cmd_buf[BUFSIZE];
    int stop_flag = 0;
    while (stop_flag != 1){
        nevents = epoll_wait(epfd, events, EVENT_NUM, -1);
        if (nevents < 0)
        unix_error("Error!");
        for (unsigned i = 0; i < nevents; i++){
            struct epoll_event eev = events[i];
            int tmpfd = (int)eev.data.fd;
            if (eev.events & EPOLLRDHUP)
            {
                rm_client(epfd, tmpfd);
                close(tmpfd);
                printf("Closed %d.\n", tmpfd);
            }
            else if (eev.events & EPOLLIN){
                if (tmpfd == sockfd){
                    clientfd = Accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
                    printf("%s:%d connected in fd %d.\n",
                        inet_ntoa(client_addr.sin_addr),
                        ntohs(client_addr.sin_port),
                        clientfd);
                    add_client(epfd, clientfd);
                }
                else if (tmpfd == STDIN_FILENO){
                    size_t size;
                    size = read(STDIN_FILENO, cmd_buf, BUFSIZE);
                    printf("Received cmd from stdin: %s", cmd_buf);
                    if (cmd_buf[0] == 'q'){
                        printf("Stop server.\n");
                        stop_flag = 1;
                    }
                    memset(cmd_buf, 0, BUFSIZE);
                }
                else {
                    size_t size;
                    size = recv(tmpfd, buffer, BUFSIZE, 0);
                    printf("Received %s.\n", buffer);
                    if (buffer[0] == 'q') {
                        rm_client(epfd, tmpfd);
                        close(tmpfd);
                    }
                    send(tmpfd, buffer, size, 0);
                    memset(buffer, 0, BUFSIZE);
                }
            }
        }
    }
    close(sockfd);
    return 0;
}