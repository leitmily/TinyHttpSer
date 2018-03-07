#include "handle.h"
#include "common.h"
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>


extern int server_bytes_sent;

/*-----------------------------------------------------------------
    process_rq(char *rq, int fd)
    do what the request asks for and write reply to fd
    hadnles request in a new process
    rq is HTTP command: GET /foo.bar.teml HTTP/1.0
---------------------------------------------------->  //包含了Linux C 中的函数--------------*/

void process_rq(char *rq, int fd) {
    char cmd[BUFSIZ], arg[BUFSIZ], version[BUFSIZ], cpath[BUFSIZ];
    
    //strcpy(arg, ".");      /* precede args with ./ */
    if(sscanf(rq, "%s %s %s", cmd, arg, version) != 3) return;

    sanitize(arg);//获得请求路径(请求路径)，和解码中文路径
    getcwd(cpath,BUFSIZ);//获得当前路径
    strcat(cpath, arg);
    //printf("file: %s, line: %d, sanitized version is %s\n", __FILE__, __LINE__, cpath);
    //printf("file: %s, line: %d, atg is %s\n", __FILE__, __LINE__, arg);
    if(strcmp(cmd, "GET") != 0)                          /* check command */
        not_implemented(fd);
    else if(built_in(cpath, fd))
    ;
    else if(not_exist(cpath))                             /* does the arg exist */
        do_404(arg, fd);                                /* n: tell the user */
    else if(isadir(cpath))                                /* is it a directory? */
        do_ls(arg, fd);                                 /* y: list contents */
    else                                                /* otherwise */
        do_cat(cpath, fd);                                /* display contents */
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