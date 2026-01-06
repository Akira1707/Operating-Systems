#pragma once
#include "conn.h"
#include <mqueue.h>
#include <string>

class ConnMQ : public Conn {
private:
    std::string name;
    mqd_t mq;
public:
    ConnMQ(const std::string &id, bool create);
    bool Read(void *buf, size_t count) override;
    bool Write(void *buf, size_t count) override;
    ~ConnMQ();
};