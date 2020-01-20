#include "csapp.h"
#include <sys/event.h>


#define MY_PORT   9999
#define BUFSIZE   1024
#define EVENT_NUM 20
#define THREAD_POOL_SIZE 10
#define MAX_TASK_BUF_SIZE 1024
#define READ_TASK 0
#define ACCEPT_TASK 1


int Kevent(int kq,
    const struct kevent *changelist, int nchanges,
    struct kevent *eventlist, int nevents,
    const struct timespec *timeout)
{
    int rc;
    if ((rc = kevent(kq, changelist, nchanges, eventlist, nevents, timeout)) < 0)
    unix_error("Kevent error");
    return rc;
}

int Kqueue(void){
    int rc;
    if ((rc = kqueue()) < 0)
    unix_error("Kqueue error");
    return rc;
}


int kq;

typedef struct {
    int fd;
    int type;
} task_t;

typedef struct {
    task_t tasks[MAX_TASK_BUF_SIZE];
    int rear;
    int head;
    sem_t slots;
    sem_t items;
    sem_t mutex;
} task_buf_t;

void rm_task_buf(task_buf_t *task_buf, task_t *task){
    P(&task_buf->items);
    P(&task_buf->mutex);
    task->fd = task_buf->tasks[task_buf->rear].fd;
    task->type = task_buf->tasks[task_buf->rear].type;
    task_buf->rear = (task_buf->rear + 1 ) % MAX_TASK_BUF_SIZE;
    V(&task_buf->mutex);
    V(&task_buf->slots); 
}

void inst_task_buf(task_buf_t *task_buf, int fd, int task_type){
    P(&task_buf->slots);
    P(&task_buf->mutex);
    task_buf->tasks[task_buf->head].fd = fd;
    task_buf->tasks[task_buf->head].type = task_type;
    task_buf->head = (task_buf->head + 1 ) % MAX_TASK_BUF_SIZE;
    V(&task_buf->mutex);
    V(&task_buf->items); 
}

void init_kqueue(int kq, int sockfd, struct kevent *conn_and_cmd_event){
    EV_SET(&conn_and_cmd_event[0], sockfd, EVFILT_READ, EV_ADD, 0, 20, NULL);
    EV_SET(&conn_and_cmd_event[1], STDIN_FILENO, EVFILT_READ, EV_ADD, 0, 0, NULL);
    Kevent(kq, conn_and_cmd_event, 2, NULL, 0, NULL);
}

void rm_client(int kq, int fd){
    struct kevent tmp;
    EV_SET(&tmp, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    Kevent(kq, &tmp, 1, NULL, 0, NULL);
}

void handler(task_buf_t *task_buf){
    pthread_detach(pthread_self());
    char buffer[BUFSIZE];
    size_t size;
    while (1){
        task_t task;
        rm_task_buf(task_buf, &task);
        size = recv(task.fd, buffer, BUFSIZE, 0);
        printf("Received %s", buffer);
        if (buffer[0] == 'q') {
            rm_client(kq, task.fd);
            close(task.fd);
            return;
        }
        send(task.fd, buffer, size, 0);
        memset(buffer, 0, BUFSIZE);
    }
}

void acceptor(struct sockaddr_in * client_addr, int sockfd, int *addrlen, int kq){
    int clientfd = Accept(sockfd, (struct sockaddr*)&client_addr, addrlen);
    printf("%s:%d connected in fd %d\n",
            inet_ntoa(client_addr->sin_addr),
            ntohs(client_addr->sin_port),
            clientfd);
    struct kevent tmp;
    EV_SET(&tmp, clientfd, EVFILT_READ, EV_ADD, 0, 0, NULL);
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

    struct kevent conn_and_cmd_event[2];
    struct kevent events[EVENT_NUM];
    kq = Kqueue();

    int nevents;

    init_kqueue(kq, sockfd, conn_and_cmd_event);
    char cmd_buf[BUFSIZE];
    int stop_flag = 0;
    int i;
    task_buf_t task_buf;
    Sem_init(&task_buf.slots, 0, MAX_TASK_BUF_SIZE);
    Sem_init(&task_buf.items, 0, 0);
    Sem_init(&task_buf.mutex, 0, 1);
    for (i = 0; i < THREAD_POOL_SIZE; i++){
        pthread_t tid;
        pthread_create(&tid, NULL, handler, &task_buf);
    }
    while (stop_flag != 1){
        nevents = kevent(kq, NULL, 0, events, EVENT_NUM, NULL);
        if (nevents < 0)
        unix_error("Error!");
        for (unsigned i = 0; i < nevents; i++){
            struct kevent kev = events[i];
            int tmpfd = (int)kev.ident;
            if (tmpfd == sockfd){
                acceptor(&client_addr, sockfd, addrlen, kq);
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