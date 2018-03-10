#ifndef COMMON_H
#define COMMON_H

#include <pthread.h>
#include <stdio.h>


/*
 * initialize the status variables and
 * set the thread attribute to detached 
 */
void setup(pthread_attr_t *attrp);

void *handle_call(void *fdptr);

/*-----------------------------------------------------------------
    skip_rest_of_header(FILE *)
    skip over all request info until a CCRNL is seen
------------------------------------------------------------------*/
void skip_rest_of_header(FILE *fp);

/*-----------------------------------------------------------------
    read content length in head at the post method.
------------------------------------------------------------------*/
int read_content_length(FILE *fp);

/*
 * make sure all paths are below the current directory
 */
void sanitize(char *str);

/* handle built-in URLs here. Only one so far is “status" */
int built_in(char *arg, int fd);

int http_reply(int fd, FILE **fpp, int code, const char *msg, const char *type, const char *content);

/*-----------------------------------------------------------------
    simple functions first:
        not_implemented(fd)     unimplemented HTTP command
        and do_404(item, fd)  #include <dirent.h>  no such object
------------------------------------------------------------------*/
void not_implemented(int fd);

/*-----------------------------------------------------------------
    the directory listing section 
    isadir() uses stat, not_exist() uses stat
------------------------------------------------------------------*/
bool isadir(char *f);
bool not_exist(char *f);

/*-----------------------------------------------------------------
    functions to cat files here.
    file_type(filename) returns the 'extension': cat uses it
------------------------------------------------------------------*/
char *file_type(char *f);

/*
 * input absolute path, and change work space; 
 */
void setdir(const char *abpath);

#endif // COMMON_H