/*  twebserv.cpp - a threaded minimal web server (version 0.2)
*  usage: tws portnumbe
*feature: supports the GET ommand only
*         runs in the current directory
*         creates a thread to handle each request
*         suppports a special status URL to reprot internal state
*  build: cc twebser.cpp socklib.c -lpthread -o twebserv
*/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#include "handle.h"
#include "socklib.h"
#include "common.h"
#include "./threadpool/cthreadpool.h"

/* server facts here */
time_t server_started;//请求时间
int server_bytes_sent;//发送字节总数
int server_requests; //请求总次数

int main(int argc, char **argv) {
    if(argc < 5) {
        fprintf(stderr, "请指定端口号和工作路径！\n\
        示例:\n./twebserv -p 8888 -d /home/\n");
        exit(1);
    }

    char *dir = NULL;
    int port = -1;
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "-d") ==0)
            dir = argv[++i];
    }
    if(port == -1 || dir == NULL) {
        fprintf(stderr, "%d\n", port);
        fprintf(stderr, "%s\n", dir);
        fprintf(stderr, "输入参数不正确，请重新输入！\n");
        fprintf(stderr, "请指定端口号和工作路径！\n\
        示例: ./twebserv -p 8888 -d /home/\n");
        exit(1);
    }

    chdir(dir);
    int sock, fd;
    int *fdptr;
    // pthread_t worker;
    // pthread_attr_t attr;

    void *handle_call(void *);

    CThreadPool threadpool(5);
    

    sock = make_server_socket(port);
    if(sock == -1) {
        perror("making socket");
        exit(2);
    }

    // setup(&attr);//置独立线程，即线程结束后无需调用pthread_join阻塞等待线程结束，忽略SIGPIPE信号

    /* main loop here: take call, handle call in new thread */
    while(true) {
        fd = accept(sock, NULL, NULL);
        server_requests++;
        fdptr = (int *)malloc(sizeof(int));
        *fdptr = fd;
        // pthread_create(&worker, &attr, handle_call, fdptr);
        CMyTask *taskObj = new CMyTask(&handle_call, (void*)fdptr);
        threadpool.AddTask(taskObj);
    }
    return 0;
}