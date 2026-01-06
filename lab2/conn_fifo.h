#pragma once
#include "conn.h"
#include <string>
#include <fcntl.h>

class ConnFIFO : public Conn {
private:
    std::string name;
    int fd;
public:
    ConnFIFO(const std::string &id, bool create);
    bool Read(void *buf, size_t count) override;
    bool Write(void *buf, size_t count) override;
    ~ConnFIFO();
};