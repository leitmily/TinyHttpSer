#include "cepoll.h"

Epoll::Epoll(int size, int max_events) : max_events(max_events) {
    evlist = new struct epoll_event;
    EpollCreate(size);
}

Epoll::~Epoll() {
    delete[] evlist;
}

int Epoll::EpollCreate(int size) {
    epoll_fd = epoll_create(size);
    if(epoll_fd == -1) {
        errExit("epoll create error");
        return -1;
    }
    return 0;
}

int Epoll::AddETIN(int fd) const {
    struct epoll_event ev;
    ev.events = ETIN;
    ev.data.fd = fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        errExit("add read event failed");
        return -1;
    }
    return 0;
}

int Epoll::AddETOUT(int fd) const {
    struct epoll_event ev;
    ev.events = ETOUT;
    ev.data.fd = fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        errExit("add write event failed");
        return -1;
    }
    return 0;
}

int Epoll::ModEvent(int fd, ETOP op) const {
    struct epoll_event ev;
    ev.events = op;
    ev.data.fd = fd;
    if(epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        errExit("modify epoll event failed");
        return -1;
    }
    return 0;
}

int Epoll::WaitEvent(int timeout) {
    int ready;
    // readyFd.clear();
    ready = epoll_wait(epoll_fd, evlist, max_events, timeout);
    if(ready == -1) {
        errExit("epoll_wait error");
        return -1;
    }
    if(ready == 0) {
        errExit("beyond time");
        return 0;
    }
    // for(int j = 0; j < ready; ++j) {
    //     //if(evlist[j].events & EPOLLIN)
    //         readyFd.push_back(evlist[j].data.fd);
    // }
    return ready;
}

int Epoll::GetEventFd(int n) {
    return evlist[n].data.fd;
}

bool Epoll::isReadAvailable(int n) {
    return evlist[n].events & EPOLLIN ? true : false;
}

bool Epoll::isWriteAvailable(int n) {
    return evlist[n].events & EPOLLOUT ? true : false;
}