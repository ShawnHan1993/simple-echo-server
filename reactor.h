#include "csapp.h"

#define MAX_TASK_BUF_SIZE 1024
#define READ_TASK 0
#define ACCEPT_TASK 1

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

void init_task_buf(task_buf_t *task_buf){
    task_buf->head = 0;
    task_buf->rear = 0;
    Sem_init(&task_buf->slots, 0, MAX_TASK_BUF_SIZE);
    Sem_init(&task_buf->items, 0, 0);
    Sem_init(&task_buf->mutex, 0, 1);
}