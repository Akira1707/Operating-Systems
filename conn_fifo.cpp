#include "conn_fifo.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <errno.h>
#include <sys/select.h>

ConnFIFO::ConnFIFO(const std::string &id, bool create) : Conn(id, create), name(id) {
    if(create) mkfifo(name.c_str(), 0666);
    fd = open(name.c_str(), O_RDWR);
    if(fd == -1) { perror("open fifo"); exit(1);}
}

bool ConnFIFO::Read(void *buf, size_t count) {
    fd_set set;
    FD_ZERO(&set); FD_SET(fd,&set);
    struct timeval tv={5,0};
    int rv = select(fd+1,&set,NULL,NULL,&tv);
    if(rv>0) {
        ssize_t r = read(fd, buf, count);
        return r == (ssize_t)count;
    } else {
        std::cerr << "FIFO read timeout\n";
        return false;
    }
}

bool ConnFIFO::Write(void *buf, size_t count) {
    ssize_t r = write(fd, buf, count);
    return r == (ssize_t)count;
}

ConnFIFO::~ConnFIFO() {
    close(fd);
    unlink(name.c_str());
}