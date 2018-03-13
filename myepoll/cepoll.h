#ifndef CEPOLL_H
#define CEPOLL_H

#include <sys/epoll.h>
#include <deque>

#include "../errexit.h"
#include "../threadpool/mutex.h"

class Epoll {
public:
    Epoll(int size = 30, int max_events = 5); // 初始化个数，最大就绪态文件描述符的个数,等待时间
    ~Epoll();
    int EpollCreate(int size); // 设置初始监听文件描述符个数
    int AddETIN(int fd) const;
    int AddETOUT(int fd) const;
    int ModEvent(int fd, int ETOP) const;
    int WaitEvent(int timeout);
    int GetEventFd(int n);
    bool isReadAvailable(int n);
    bool isWriteAvailable(int n);

    typedef int ETOP;
    static const ETOP ETIN = EPOLLIN | EPOLLET;
    static const ETOP ETOUT = EPOLLOUT | EPOLLET;
    static const ETOP IN = EPOLLIN;
    static const ETOP OUT = EPOLLOUT;
    static const ETOP ET = EPOLLET;
    

private:
    int epoll_fd;
    struct epoll_event *evlist; // 存储就绪态文件描述符信息
    // std::deque<int> readyFd;
    int max_events;
    Mutex emutex;

};

#endif  // CEPOLL_H