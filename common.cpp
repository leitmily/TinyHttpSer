#include "common.h"
#include <stdlib.h>
#include "handle.h"
#include <sys/stat.h>
#include <string.h>

extern time_t server_started;
extern int server_bytes_sent;
extern int server_requests;

/*
 * initialize the status variables and
 * set the thread attribute to detached 
 */
void setup(pthread_attr_t *attrp) {//设置独立线程，即线程结束后无需调用pthread_join阻塞等待线程结束
    pthread_attr_init(attrp);
    pthread_attr_setdetachstate(attrp, PTHREAD_CREATE_DETACHED);

    time(&server_started);
    server_requests = 0;
    server_bytes_sent = 0;
}

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

/*-----------------------------------------------------------------
    the directory listing section 
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

/*-----------------------------------------------------------------
    functions to cat files here.
    file_type(filename) returns the 'extension': cat uses it
------------------------------------------------------------------*/

char *file_type(char *f) {
    char *cp;
    if((cp = strrchr(f, '.')) != NULL) return cp + 1;
    return " -";
}

/*
 * input absolute path, and change work space; 
 */
void setdir(const char *abpath) {
    if(abpath == NULL) {
        
    }
}