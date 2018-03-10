#ifndef HANDLE_H
#define HANDLE_H

#include <stdio.h>
/*-----------------------------------------------------------------
    process_rq(char *rq, int fd)
    do what the request asks for and write reply to fd
    hadnles request in a new process
    rq is HTTP command: GET /foo.bar.teml HTTP/1.0
------------------------------------------------------------------*/
void process_rq(char *rq, int fd, FILE *fpin);
void do_404(char *item, int fd);
void do_ls(char *dir, int fd);

/* di_car(filename, fd): sends header then the contents */
void do_cat(char *f, int fd);

void execute_cgi(int fd, FILE *fpin, const char *path, const char *method, const char *query_string);

#endif