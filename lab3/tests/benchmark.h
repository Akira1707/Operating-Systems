#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdio.h>
#include <pthread.h>
#include <vector>
#include <sys/time.h>
#include <stdlib.h>
#include <algorithm>
#include "../sets/fine_set.h"
#include "../queues/mcsp_queue.h"

long time_ms() {
    struct timeval t;
    gettimeofday(&t, nullptr);
    return t.tv_sec*1000 + t.tv_usec/1000;
}

// ---------------- FineSet benchmark ----------------

struct BenchSetArg {
    FineSet* set;
    std::vector<int>* data;
};

void* bench_set_thread(void* arg) {
    BenchSetArg* a = (BenchSetArg*)arg;
    for(int val : *a->data) a->set->add(val);
    return nullptr;
}

// Randomized / Fixed benchmark for FineSet
void benchmark_sets_mt(int producers, int n, int runs, bool randomized=true) {
    printf("Starting multi-threaded benchmark for FineSet (%s)\n", randomized?"Randomized":"Fixed");
    std::vector<int> values(n);
    for(int i=0;i<n;i++) values[i]=i;

    long total_time = 0;
    for(int r=0;r<runs;r++) {
        FineSet s;
        std::vector<std::vector<int>> thread_data(producers);

        // prepare per-thread data
        if(randomized) std::random_shuffle(values.begin(), values.end());
        int chunk = n / producers;
        for(int i=0;i<producers;i++) {
            int start = i*chunk;
            int end = (i==producers-1)? n : (i+1)*chunk;
            thread_data[i].assign(values.begin()+start, values.begin()+end);
        }

        pthread_t threads[producers];
        BenchSetArg args[producers];
        long start_time = time_ms();
        for(int i=0;i<producers;i++) {
            args[i] = {&s, &thread_data[i]};
            pthread_create(&threads[i], nullptr, bench_set_thread, &args[i]);
        }
        for(int i=0;i<producers;i++) pthread_join(threads[i], nullptr);
        long end_time = time_ms();
        printf("Run %d: %ld ms\n", r+1, end_time-start_time);
        total_time += (end_time-start_time);
    }
    printf("Average: %.2f ms\n", total_time*1.0/runs);
}

// ---------------- MCSPQueue benchmark ----------------

struct BenchQueueArg {
    MCSPQueue<int>* q;
    std::vector<int>* data;
};

void* bench_queue_thread(void* arg) {
    BenchQueueArg* a = (BenchQueueArg*)arg;
    for(int val : *a->data) a->q->enqueue(val);
    return nullptr;
}

void benchmark_mcsp_queue(int consumers, size_t capacity, int n, int runs, bool randomized=true) {
    printf("Starting multi-threaded benchmark for MCSPQueue (%s)\n", randomized?"Randomized":"Fixed");
    std::vector<int> values(n);
    for(int i=0;i<n;i++) values[i]=i;

    long total_time = 0;
    for(int r=0;r<runs;r++) {
        MCSPQueue<int> q(capacity);

        std::vector<int> thread_data = values; 
        if(randomized) std::random_shuffle(thread_data.begin(), thread_data.end());

        pthread_t producer_thread;
        BenchQueueArg prod_arg = {&q, &thread_data};
        long start_time = time_ms();
        pthread_create(&producer_thread, nullptr, bench_queue_thread, &prod_arg);

        // 1 producer, multiple consumers
        int num_consumers = consumers;
        pthread_t c_threads[num_consumers];
        for(int i=0;i<num_consumers;i++) {
            QThreadArg c_arg = {&q, 0, 0};
            pthread_create(&c_threads[i], nullptr, q_reader_thread, &c_arg);
        }

        pthread_join(producer_thread, nullptr);
        for(int i=0;i<num_consumers;i++) pthread_join(c_threads[i], nullptr);

        long end_time = time_ms();
        printf("Run %d: %ld ms\n", r+1, end_time-start_time);
        total_time += (end_time-start_time);
    }
    printf("Average: %.2f ms\n", total_time*1.0/runs);
}

// ---------------- Legacy single-thread benchmark ----------------

struct BenchArg {
    FineSet* set;
    int start, end;
};

void* bench_writer_thread(void* arg) {
    BenchArg* a = (BenchArg*)arg;
    for(int i=a->start;i<a->end;i++) a->set->add(i);
    return nullptr;
}

void benchmark_sets(int producers, int consumers, int n, int runs) {
    printf("Starting single-type benchmark for FineSet\n");
    int chunk = n / producers;
    for(int r=0;r<runs;r++) {
        FineSet s;
        pthread_t threads[producers];
        BenchArg args[producers];
        long start = time_ms();
        for(int i=0;i<producers;i++) {
            args[i] = {&s, i*chunk, (i+1)*chunk};
            pthread_create(&threads[i], nullptr, bench_writer_thread, &args[i]);
        }
        for(int i=0;i<producers;i++) pthread_join(threads[i], nullptr);
        long end = time_ms();
        printf("Run %d: %ld ms\n", r+1, end-start);
    }
}

#endif


