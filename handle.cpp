#include "handle.h"
#include "common.h"
#include <string.h>
#include <dirent.h>


extern int server_bytes_sent;

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
    

    
void do_404(char *item, int fd){
    http_reply(fd, NULL, 404, "Not Foud", "text/plain", 
        "The item you seek is not here");
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