#ifndef TEST_QUEUES_H
#define TEST_QUEUES_H

#include <stdio.h>
#include <pthread.h>
#include "../queues/mcsp_queue.h"

struct QThreadArg {
    MCSPQueue<int>* q;
    int start, end;
};

void* q_writer_thread(void* arg) {
    QThreadArg* a = (QThreadArg*)arg;
    for(int i=a->start;i<a->end;i++) a->q->enqueue(i);
    return nullptr;
}

void* q_reader_thread(void* arg) {
    QThreadArg* a = (QThreadArg*)arg;
    int val;
    while(a->q->dequeue(val)) {}
    return nullptr;
}

void run_queue_combined_test_impl(int impl, size_t capacity, int producers, int consumers, int n) {
    if(producers > 1) {
        printf("Warning: MCSPQueue supports only 1 producer, reducing producers to 1\n");
        producers = 1;
    }

    MCSPQueue<int> q(capacity);
    pthread_t p_threads[producers], c_threads[consumers];
    int chunk = n / producers;

    QThreadArg p_args[producers];
    for(int i=0;i<producers;i++) {
        p_args[i] = {&q, i*chunk, (i+1)*chunk};
        pthread_create(&p_threads[i], nullptr, q_writer_thread, &p_args[i]);
    }

    QThreadArg c_args[consumers];
    for(int i=0;i<consumers;i++) {
        c_args[i] = {&q, 0, 0};
        pthread_create(&c_threads[i], nullptr, q_reader_thread, &c_args[i]);
    }

    for(int i=0;i<producers;i++) pthread_join(p_threads[i], nullptr);
    for(int i=0;i<consumers;i++) pthread_join(c_threads[i], nullptr);

    printf("Queue combined test finished, empty=%d\n", q.empty());
}

#endif



