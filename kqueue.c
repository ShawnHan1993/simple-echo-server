#include "csapp.h"


#define MY_PORT   9999
#define BUFSIZE   1024
#define EVENT_NUM 20


typedef struct {
    int active_fd[FD_SETSIZE];
    int nread;
    fd_set read_set;
    fd_set ready_set;
    int maxfd;
} client_pool_t;


void init_kqueue(int kq, int sockfd, struct kevent *conn_and_cmd_event){
    EV_SET(&conn_and_cmd_event[0], sockfd, EVFILT_READ, EV_ADD, 0, 20, NULL);
    EV_SET(&conn_and_cmd_event[1], STDIN_FILENO, EVFILT_READ, EV_ADD, 0, 0, NULL);
    Kevent(kq, conn_and_cmd_event, 2, NULL, 0, NULL);
}

void rm_client(int kq, size_t fd){
    struct kevent tmp;
    EV_SET(&tmp, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    Kevent(kq, &tmp, 1, NULL, 0, NULL);
}

void add_client(int kq, int fd){
    struct kevent tmp;
    EV_SET(&tmp, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    Kevent(kq, &tmp, 1, NULL, 0, NULL);
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


    int kq;
    struct kevent conn_and_cmd_event[2];
    struct kevent events[EVENT_NUM];
    kq = Kqueue();

    int nevents;

    init_kqueue(kq, sockfd, conn_and_cmd_event);
    char cmd_buf[BUFSIZE];
    int stop_flag = 0;
    while (stop_flag != 1){
        nevents = kevent(kq, NULL, 0, events, EVENT_NUM, NULL);
        if (nevents < 0)
        unix_error("Error!");
        for (unsigned i = 0; i < nevents; i++){
            struct kevent kev = events[i];
            int tmpfd = (int)kev.ident;
            if (tmpfd == sockfd){
                clientfd = Accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
                printf("%s:%d connected in fd %d\n",
                    inet_ntoa(client_addr.sin_addr),
                    ntohs(client_addr.sin_port),
                    clientfd);
                add_client(kq, clientfd);
            }
            else if (tmpfd == STDIN_FILENO){
                size_t size;
                size = read(STDIN_FILENO, cmd_buf, BUFSIZE);
                printf("Received cmd from stdin: %s", cmd_buf);
                if (cmd_buf[0] == 'q'){
                    printf("Stop server.");
                    stop_flag = 1;
                }
                memset(cmd_buf, 0, BUFSIZE);
            }
            else {
                size_t size;
                size = recv(tmpfd, buffer, BUFSIZE, 0);
                printf("Received %s", buffer);
                if (buffer[0] == 'q') {
                    rm_client(kq, tmpfd);
                    close(tmpfd);
                }
                send(tmpfd, buffer, size, 0);
                memset(buffer, 0, BUFSIZE);
            }
        }
    }
    close(sockfd);
    return 0;
}