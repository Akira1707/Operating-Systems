#pragma once
#include "conn.h"
#include <string>

class ConnSock : public Conn {
private:
    int sockfd;
public:
    ConnSock(const std::string &port, bool create);
    bool Read(void *buf, size_t count) override;
    bool Write(void *buf, size_t count) override;
    ~ConnSock();
};