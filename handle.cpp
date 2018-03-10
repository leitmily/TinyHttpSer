#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>

#include "handle.h"
#include "common.h"


extern int server_bytes_sent;

/*-----------------------------------------------------------------
    process_rq(char *rq, int fd)
    do what the request asks for and write reply to fd
    hadnles request in a new process
    rq is HTTP command: GET /foo.bar.teml HTTP/1.0
---------------------------------------------------->  //包含了Linux C 中的函数--------------*/

void process_rq(char *rq, int fd, FILE *fpin) {
    char cmd[BUFSIZ], url[BUFSIZ], version[BUFSIZ], cpath[BUFSIZ], query_string[BUFSIZ] = "\0";
    char path[BUFSIZ];
    
    if(sscanf(rq, "%s %s %s", cmd, url, version) != 3) return;
    strcpy(path, url);

    char *arg = strchr(path, '?');
    if (arg != NULL)
    {
        *arg = '\0';
        strcpy(query_string, ++arg);
    }
    sanitize(path);//获得请求路径(请求路径)，和解码中文路径
    getcwd(cpath,BUFSIZ);//获得当前路径
    strcat(cpath, path); //完整路径

    if(strcmp(cmd, "GET") == 0) {
        skip_rest_of_header(fpin);//忽略请求头部
        if(built_in(cpath, fd))
        ;
        else if (not_exist(cpath)) /* does the arg exist */
            do_404(url, fd);       /* n: tell the user */
        else if (isadir(cpath))    /* is it a directory? */
            do_ls(url, fd);        /* y: list contents */
        else {                     /* otherwise */
            if(query_string[0] == '\0')
                do_cat(cpath, fd);
            else
                execute_cgi(fd, fpin, cpath, cmd, query_string);
        }
    }
    else if(strcmp(cmd, "POST") == 0) {
        if(built_in(cpath, fd))
        ;
        else {
            execute_cgi(fd, fpin, cpath, cmd, query_string);
        }
    }
    else
        not_implemented(fd);
}   

    
void do_404(char *item, int fd){
    http_reply(fd, NULL, 404, "Not Foud", "text/html", 
        "<HTML><TITLE>Not Found</TITLE>\r\n<BODY><P>The item you seek is not here\r\n</BODY></HTML>\r\n");
}    // getcwd(cpath,BUFSIZ);//获得当前路径
    // strcat(cpath, arg);
    

void do_ls(char *dir, int fd) {
    char cpath[BUFSIZ];
    getcwd(cpath,BUFSIZ);//获得当前路径
    strcat(cpath, dir);
    //printf("file: %s, line: %d, path is %s\n", __FILE__, __LINE__, cpath);
    DIR *dirptr;
    struct dirent *direntp;
    struct stat st;
    FILE *fp;
    int bytes = 0;

    bytes = http_reply(fd, &fp, 200, "OK", "text/html", NULL);
    bytes += fprintf(fp, "<HTML><TITLE>%s</TITLE>\r\n<BODY>", dir);
    //bytes += fprintf(fp, "Listing of Directory %s\n", dir);
    if((dirptr = opendir(cpath)) != NULL) {
        while((direntp = readdir(dirptr)) != NULL) {
            char tp[BUFSIZ];
            strcpy(tp, cpath);
            strcat(tp, direntp->d_name);
            lstat(tp,&st);//获取文件信息，跟stat区别：stat查看符号链接所指向的文件信息，而lstat则返回符号链接的信息
            if(S_ISDIR(st.st_mode)) {
                if(strcmp(".", direntp->d_name) == 0 || strcmp("..", direntp->d_name) == 0) 
                    continue;
                bytes += fprintf(fp, "&nbsp;<A href=\"%s/\">%s</A>&nbsp;&nbsp;[DIR]<br/>", direntp->d_name, direntp->d_name);
            }
            else {
                bytes += fprintf(fp, "&nbsp;<A href=\"%s\">%s</A>&nbsp;&nbsp;%ld&nbsp;bytes<br/>", 
                    direntp->d_name, direntp->d_name, st.st_size);
            }
            
        }
        closedir(dirptr);
    }
    bytes += fprintf(fp, "</body></html>");
    fclose(fp);
    server_bytes_sent += bytes;
}

/* di_car(filename, fd): sends header then the contents */

void do_cat(char *f, int fd) {
    char *extension = file_type(f);
    const char *type = "text/plain";
    FILE *fpsock, *fpfile;
    int c, bytes = 0;

    if( strcmp(extension, "html") == 0)
        type = "text/html";
    else if(strcmp(extension, "htm") == 0)
        type = "image/html";
    else if(strcmp(extension, "gif") == 0)
        type = "image/gif";
    else if(strcmp(extension, "jpg") == 0)
        type = "image/jpeg";
    else if(strcmp(extension, "jpeg") == 0)
        type = "image/jpeg";
    else if(strcmp(extension, "png") == 0)
        type = "image/png";
    else if(strcmp(extension, "bmp") == 0)
        type = "image/x-xbitmap";
    
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

void execute_cgi(int fd, FILE *fpin, const char *path, const char *method, const char *query_string) {
    char buf[BUFSIZ];
    char bufPost[BUFSIZ];
    int cgi_output[2];
    int cgi_input[2];

    pid_t pid;
    int status, numchars = 1, content_length = 0;
    
    if(strcmp(method, "POST") == 0) {
        content_length = read_content_length(fpin);
        // recv(client, &c, 1, 0);
        fgets(bufPost, BUFSIZ, fpin);
        printf("bufPost is %s\n", bufPost);
    }



    /* 建立管道*/
    if (pipe(cgi_output) < 0) {
        /*错误处理*/
        not_implemented(fd);
        return;
    }
    /*建立管道*/
    if (pipe(cgi_input) < 0) {
        /*错误处理*/
        not_implemented(fd);
        return;
    }


    if ((pid = fork()) < 0 ) {
        /*错误处理*/
        not_implemented(fd);
        return;
    }

    if(pid == 0) {// child process
        fclose(fpin);
        close(fd);
        char meth_env[BUFSIZ];
        char query_env[BUFSIZ];
        char length_env[BUFSIZ];
        /* 把 STDOUT 重定向到 cgi_output 的写入端 */
        dup2(cgi_output[1], STDOUT_FILENO); //STDOUT_FILENO = 1
        /* 把 STDIN 重定向到 cgi_input 的读取端 */
        dup2(cgi_input[0], STDIN_FILENO); //STDIN_FILENO = 0
        /* 关闭 cgi_input 的写入端 和 cgi_output 的读取端 */
        close(cgi_output[0]);
        close(cgi_input[1]);
        /*设置 request_method 的环境变量*/
        sprintf(meth_env, "REQUEST_METHOD=%s", method);
        putenv(meth_env);
        if (strcasecmp(method, "GET") == 0) {
            /*设置 query_string 的环境变量*/
            sprintf(query_env, "QUERY_STRING=%s", query_string);
            putenv(query_env);
        }
        else {   /* POST */
            /*设置 content_length 的环境变量*/
            sprintf(length_env, "CONTENT_LENGTH=%d", content_length);
            putenv(length_env);
        }
        execl(path, path, (char*)NULL);
        exit(1);
    }
    else { // father process
        /* 关闭 cgi_input 的读取端 和 cgi_output 的写入端 */
        close(cgi_output[1]);
        close(cgi_input[0]);

        if (strcasecmp(method, "POST") == 0) {
            /*把 POST 数据写入 cgi_input，现在重定向到 STDIN */
            write(cgi_input[1], bufPost, content_length);
        }

        FILE *fpout = fdopen(fd, "w");
        fprintf(fpout, "HTTP/1.0 200 OK\r\n");
        fflush(fpout);

        while (read(cgi_output[0], buf, 1) > 0) {
            putchar(buf[0]);
            fflush(stdout);
            send(fd, buf, 1, 0);
        }

        /*关闭管道*/
        close(cgi_output[0]);
        close(cgi_input[1]);
        /*等待子进程*/
        waitpid(pid, &status, 0);
    }
}    
