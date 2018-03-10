#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "common.h"
#include "handle.h"

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

    time(&server_started);//请求时间
    server_requests = 0;
    server_bytes_sent = 0;


    //屏蔽SIGPIPE信号
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;//忽略信号
    sa.sa_flags = SA_NODEFER;//
    if(sigemptyset(&sa.sa_mask) == -1 || //初始化信号集为空
        sigaction(SIGPIPE, &sa, 0) == -1) { //屏蔽SIGPIPE
        perror("failed to ignore SIGPIPE");
        exit(EXIT_FAILURE);
    }
}

void *handle_call(void *fdptr) {
    //在线程中阻塞SIGPIPE信号，让主线程处理该线程
    sigset_t sgmask;
    sigemptyset(&sgmask);
    sigaddset(&sgmask, SIGPIPE);//添加要被阻塞的信号
    int t = pthread_sigmask(SIG_BLOCK, &sgmask, NULL);
    if(t != 0) {
        printf("file: %s, line: %d, block sigpipe error\n", __FILE__, __LINE__);
    }

    FILE *fpin;
    char request[BUFSIZ];
    int fd;

    fd = *(int *)fdptr;
    free(fdptr);

    fpin = fdopen(fd, "r");
    printf("开始获取http请求行.\n");
    fgets(request, BUFSIZ, fpin);//读取整行，遇到回车符结束
    printf("got a call on %d: request = %s", fd, request);
    //skip_rest_of_header(fpin);//忽略请求头部

    process_rq(request, fd, fpin);//处理请求
    printf("请求处理完成。\n");
    close(fd);
    fclose(fpin);
    return NULL;
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
    read content length in head at the post method.
------------------------------------------------------------------*/
int read_content_length(FILE *fp) {
    int length = 0;
    char  buf[BUFSIZ] = "";
    while(fgets(buf, BUFSIZ, fp) != NULL && strcmp(buf, "\r\n") != 0) {
        char *arg = strchr(buf, ':');
        if (arg == NULL)
            continue;
        *arg = '\0';
        if(strcasecmp(buf, "Content-Length") == 0)
            length = atoi(++arg);
    }
    return length;
}

/*
 * make sure all paths are below the current directory
 */
void sanitize(char *str) {
    char *src, *dest;
    src = dest = str;

    while(*src) {
        if(strncmp(src, "/../", 4) == 0) 
            src += 3;
        else if(strncmp(src, "//", 2) == 0)
            src++;
        else if(strncmp(src, "/./", 3) == 0)
            src += 2;
        else
            *dest++ = *src++;
    }
    *dest = '\0';
    // if(*str == '/')
    //     strcpy(str, str + 1);
    
    if(str[0] == '\0' || strcmp(str, "./") == 0
        || strcmp(str, "./..") == 0)
        strcpy(str, "/");

    //解码中文字符
    src = dest = str;
    for (; *dest != '\0'; ++dest) {
        if (*dest == '%') {
            int code;
            if (sscanf(dest+1, "%x", &code) != 1) 
                code = '?';
            *src++ = code;
            dest += 2;
        }
        else {
            *src++ = *dest;
        }
    }
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

int http_reply(int fd, FILE **fpp, int code, const char *msg, const char *type, const char *content) {
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
    return (char *)"/";
}

/*
 * input absolute path, and change work space; 
 */
void setdir(const char *abpath) {
    if(abpath == NULL) {

    }
}