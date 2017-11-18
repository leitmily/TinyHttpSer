/*  twebserv.cpp - a threaded minimal web server (version 0.2)
*  usage: tws portnumbe
*feature: supports the GET ommand only
*         runs in the current directory
*         creates a thread to handle each request
*         suppports a special status URL to reprot internal state
*  build: cc twebser.cpp socklib.c -lpthread -o twebserv
*/

#include "handle.h"
#include "socklib.h"
#include "common.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>

/* server facts here */
time_t server_started;
int server_bytes_sent;
int server_requests;

int main(int ac, char **av) {
    int sock, fd;
    int *fdptr;
    pthread_t worker;
    pthread_attr_t attr;

    void *handle_call(void *);

    if(ac == 1) {
        fprintf(stderr, "usage: tws portnum\n");
        exit(1);
    }

    sock = make_server_socket(atoi(av[1]));
    if(sock == -1) {
        perror("making socket");
        exit(2);
    }

    setup(&attr);

    /* main loop here: take call, handle call in new thread */
    while(true) {
        fd = accept(sock, NULL, NULL);
        server_requests++;
        fdptr = (int *)malloc(sizeof(int));
        *fdptr = fd;
        pthread_create(&worker, &attr, handle_call, fdptr);
    }
    return 0;
}