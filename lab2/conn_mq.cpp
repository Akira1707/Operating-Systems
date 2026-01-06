#include "conn_mq.h"
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <errno.h>
#include <unistd.h>
#include <chrono>

ConnMQ::ConnMQ(const std::string &id, bool create) : Conn(id, create), name(id) {
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = 100;           
    attr.mq_msgsize = sizeof(int);  
    attr.mq_curmsgs = 0;

    if(create) {
        mq_unlink(name.c_str());
        mq = mq_open(name.c_str(), O_CREAT | O_RDWR, 0666, &attr);
    } else {
        mq = mq_open(name.c_str(), O_RDWR);
    }

    if(mq == (mqd_t)-1) {
        perror("mq_open");
        exit(1);
    }
}

bool ConnMQ::Read(void *buf, size_t count) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5; // timeout 5s
    if(mq_timedreceive(mq, (char*)buf, count, nullptr, &ts) == -1) {
        perror("mq_receive");
        return false;
    }
    return true;
}

bool ConnMQ::Write(void *buf, size_t count) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 5; // timeout 5s
    if(mq_timedsend(mq, (char*)buf, count, 0, &ts) == -1) {
        perror("mq_send");
        return false;
    }
    return true;
}

ConnMQ::~ConnMQ() {
    mq_close(mq);
    mq_unlink(name.c_str());
}