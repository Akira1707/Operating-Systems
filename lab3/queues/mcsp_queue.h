#ifndef MCSP_QUEUE_H
#define MCSP_QUEUE_H

#include <atomic>
#include <pthread.h>
#include <stdlib.h>

template<typename T>
class MCSPQueue {
private:
    T* buffer;
    size_t cap;
    std::atomic<size_t> head;
    std::atomic<size_t> tail;

public:
    MCSPQueue(size_t capacity) : cap(capacity), head(0), tail(0) {
        buffer = new T[cap];
    }
    ~MCSPQueue() { delete[] buffer; }

    void enqueue(T val) {
        size_t h = head.load();
        size_t t;
        while(true) {
            t = tail.load();
            if(t + 1 < cap) {
                buffer[t] = val;
                tail.store(t + 1);
                break;
            } else {
                sched_yield();
            }
        }
    }

    bool dequeue(T &val) {
        size_t h = head.load();
        while(h < tail.load()) {
            val = buffer[h];
            if(head.compare_exchange_strong(h, h+1)) return true;
        }
        return false;
    }

    bool empty() { return head.load() == tail.load(); }
};

#endif
