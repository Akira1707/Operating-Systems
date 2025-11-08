#pragma once
#include <string>
#include <cstddef>

class Conn {
public:
    Conn(const std::string &id, bool create) {}
    virtual bool Read(void *buf, size_t count) = 0;
    virtual bool Write(void *buf, size_t count) = 0;
    virtual ~Conn() {}
};
