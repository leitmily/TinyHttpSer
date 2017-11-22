/*  twebserv.cpp - a threaded minimal web server (version 0.2)
*  usage: tws portnumbe
*feature: supports the GET ommand only
*         runs in the current directory
*         creates a thread to handle each request
*         suppports a special status URL to reprot internal state
*  build: cc twebser.cpp socklib.c -lpthread -o twebserv
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <pthread.h>
#include <dirent.h>

/* server facts here */
time_t server_started;
int server_bytes_sent;
int server_requests;

extern int make_server_socket(int);
void setup(pthread_attr_t *attrp);
void process_rq(char *rq, int fd);
void cannot_do(int fd);
void do_404(char *item, int fd);
bool isadir(char *f);
bool not_exist(char *f);
void do_ls(char *dir, int fd);
void do_cat(char *f, int fd);
int http_reply(int fd, FILE **fpp, int code, char *msg, char *type, char *content);
void skip_rest_of_header(FILE *fp);
void not_implemented(int fd);
int built_in(char *arg, int fd);
void sanitize(char *str);

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

/*
 * initialize the status variables and
 * set the thread attribute to detached 
 */
void setup(pthread_attr_t *attrp) {
    //设置独立线程，即线程结束后无需调用pthread_join阻塞等待线程结束
    pthread_attr_init(attrp);
    pthread_attr_setdetachstate(attrp, PTHREAD_CREATE_DETACHED);

    time(&server_started);//初始化统计字数和连接时间
    server_requests = 0;
    server_bytes_sent = 0;

}

void *handle_call(void *fdptr);

void *handle_call(void *fdptr) {
    FILE *fpin;
    char request[BUFSIZ];
    int fd;

    fd = *(int *)fdptr;
    free(fdptr);

    fpin = fdopen(fd, "r");
    fgets(request, BUFSIZ, fpin);
    printf("got a call on %d: request = %s", fd, request);
    skip_rest_of_header(fpin);

    process_rq(request, fd);

    fclose(fpin);
}

/*-----------------------------------------------------------------
    skip_rest_of_header(FILE *)
    skip over all request info until a CCRNL is seen
------------------------------------------------------------------*/

void skip_rest_of_header(FILE *fp) {
    char  buf[BUFSIZ] = "";
    while(fgets(buf, BUFSIZ, fp) != NULL && strcmp(buf, "\r\n") != 0);
}

/*-----------------------------------------------------------------
    process_rq(char *rq, int fd)
    do what the request asks for and write reply to fd
    hadnles request in a new process
    rq is HTTP command: GET /foo.bar.teml HTTP/1.0
------------------------------------------------------------------*/

void process_rq(char *rq, int fd) {
    char cmd[BUFSIZ], arg[BUFSIZ], version[BUFSIZ];

    //strcpy(arg, ".");      /* precede args with ./ */
    if(sscanf(rq, "%s %s %s", cmd, arg, version) != 3) return;

    sanitize(arg);
    printf("sanitized version is %s\n", arg);
    
    if(strcmp(cmd, "GET") != 0)                          /* check command */
        not_implemented(fd);
    else if(built_in(arg, fd))
    ;
    else if(not_exist(arg))                             /* does the arg exist */
        do_404(arg, fd);                                /* n: tell the user */
    else if(isadir(arg))                                /* is it a directory? */
        do_ls(arg, fd);                                 /* y: list contents */
    else                                                /* otherwise */
        do_cat(arg, fd);                                /* display contents */
}
    
/*
 * make sure all paths are below the current directory
 */
void sanitize(char *str) {
    char *src, *dest;
    src = dest = str;

    while(*src) {
        if(strncmp(src, "/../", 4) == 0) 
            src +=3;
        else if(strncmp(src, "//", 2) == 0)
            src++;
        else
            *dest++ = *src++;
    }
    *dest = '\0';
    if(*str == '/')
        strcpy(str, str + 1);
    
    if(str[0] == '\0' || strcmp(str, "./") == 0
        || strcmp(str, "./..") == 0)
        strcpy(str, ".");
}
    
/* handle built-in URLs here. Only one so far is “status" */
int built_in(char *arg, int fd) {
    FILE *fp;

    if(strcmp(arg, "status") != 0)
        return 0;
    http_reply(fd, &fp, 200, "OK", "text/plain", NULL);

    fprintf(fp, "Server started: %s", ctime(&server_started));
    fprintf(fp, "Total requests: %d\n", server_requests);
    fprintf(fp, "Bytes sent out: %d\n", server_bytes_sent);
    fclose(fp);
    return 1;
}

int http_reply(int fd, FILE **fpp, int code, char *msg, char *type, char *content) {
    FILE *fp = fdopen(fd, "w");
    int bytes = 0;
    
    if(fp != NULL) {
        bytes = fprintf(fp, "HTTP/1.0 %d %s\r\n", code, msg);
        bytes += fprintf(fp, "Content-type: %s\r\n\r\n", type);
        if(content)
            bytes += fprintf(fp, "%s\r\n", content);
    }
    fflush(fp);
    if(fpp)
        *fpp = fp;
    else
        fclose(fp);
    return bytes;
}

/*-----------------------------------------------------------------
    simple functions first:
        not_implemented(fd)     unimplemented HTTP command
        and do_404(item, fd)  #include <dirent.h>  no such object
------------------------------------------------------------------*/
void not_implemented(int fd) {
    http_reply(fd, NULL, 501, "Not Implemented", "text/plain", 
        "That command is not implemented");
}
    
void do_404(char *item, int fd){
    http_reply(fd, NULL, 404, "Not Foud", "text/plain", 
        "The item you seek is not here");
}
    
/*-----------------------------------------------------------------
    the directory lilsting section 
    isadir() uses stat, not_exist() uses stat
------------------------------------------------------------------*/

bool isadir(char *f) {
     struct stat info;
     return (stat(f, &info) != -1 && S_ISDIR(info.st_mode));
}

bool not_exist(char *f) {
    struct stat info;
    return (stat(f, &info) == -1);
}

void do_ls(char *dir, int fd) {
    DIR *dirptr;
    struct dirent *direntp;
    FILE *fp;
    int bytes = 0;

    bytes = http_reply(fd, &fp, 200, "OK", "text/plain", NULL);
    bytes += fprintf(fp, "Listing of Directory %s\n", dir);

    if((dirptr = opendir(dir)) != NULL) {
        while(direntp = readdir(dirptr)) {
            bytes += fprintf(fp, "%s\n", direntp->d_name);
        }
        closedir(dirptr);
    }
    fclose(fp);
    server_bytes_sent += bytes;
}

/*-----------------------------------------------------------------
    functions to cat files here.
    file_type(filename) returns the 'extension': cat uses it
------------------------------------------------------------------*/

char *file_type(char *f) {
    char *cp;
    if((cp = strrchr(f, '.')) != NULL) return cp + 1;
    return " -";
}

/* di_car(filename, fd): sends header then the contents */

void do_cat(char *f, int fd) {
    char *extension = file_type(f);
    char *type = "text/plain";
    FILE *fpsock, *fpfile;
    int c, bytes = 0;

    if( strcmp(extension, "html") == 0)
        type = "text/html";
    else if(strcmp(extension, "gif") == 0)
        type = "image/gif";
    else if(strcmp(extension, "jpg") == 0)
        type = "image/jpeg";
    else if(strcmp(extension, "jpeg") == 0)
        type = "image/jpeg";
    
    fpsock = fdopen(fd, "w");
    fpfile = fopen(f, "r");
    if(fpsock != NULL && fpfile != NULL) {
        bytes = http_reply(fd, &fpsock, 200, "OK", type, NULL);
        while((c = getc(fpfile)) != EOF) {
            putc(c, fpsock);
            bytes++;
        }
        fclose(fpfile);
        fclose(fpsock);
    }
    server_bytes_sent += bytes;
}