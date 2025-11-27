#ifndef NODE_H
#define NODE_H

#include <pthread.h>

struct Node {
    int val;
    Node* next;
    pthread_mutex_t lock;

    Node(int v) : val(v), next(nullptr) {
        pthread_mutex_init(&lock, nullptr);
    }
    ~Node() {
        pthread_mutex_destroy(&lock);
    }
};

#endif
