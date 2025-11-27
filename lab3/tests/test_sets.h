#ifndef TEST_SETS_H
#define TEST_SETS_H

#include <stdio.h>
#include <pthread.h>
#include <vector>
#include <atomic>
#include "../sets/fine_set.h"

struct ThreadArg {
    FineSet* set;
    int start, end;
    std::atomic<int>* counter; 
};

void* writer_thread(void* arg) {
    ThreadArg* a = (ThreadArg*)arg;
    for(int i = a->start; i < a->end; i++) {
        a->set->add(i);
    }
    return nullptr;
}

void* reader_thread(void* arg) {
    ThreadArg* a = (ThreadArg*)arg;
    for(int i = a->start; i < a->end; i++) {
        if(a->set->contains(i) && a->counter) {
            a->counter[i].fetch_add(1, std::memory_order_relaxed);
        }
    }
    return nullptr;
}

void run_set_writer_test_impl(int producers, int n) {
    FineSet s;
    pthread_t threads[producers];
    int chunk = n / producers;

    ThreadArg args[producers];
    for(int i=0;i<producers;i++) {
        int start = i*chunk;
        int end = (i==producers-1)? n : (i+1)*chunk;
        args[i] = {&s, start, end, nullptr};
        pthread_create(&threads[i], nullptr, writer_thread, &args[i]);
    }
    for(int i=0;i<producers;i++) pthread_join(threads[i], nullptr);

    int count = 0;
    for(int i=0;i<n;i++) if(s.contains(i)) count++;
    printf("Writer test: %d/%d elements present\n", count, n);
}

void run_set_reader_test_impl(int consumers, int n) {
    FineSet s;
    for(int i=0;i<n;i++) s.add(i);

    pthread_t threads[consumers];
    int chunk = n / consumers;
    ThreadArg args[consumers];

    for(int i=0;i<consumers;i++) {
        int start = i*chunk;
        int end = (i==consumers-1)? n : (i+1)*chunk;
        args[i] = {&s, start, end, nullptr};
        pthread_create(&threads[i], nullptr, reader_thread, &args[i]);
    }
    for(int i=0;i<consumers;i++) pthread_join(threads[i], nullptr);

    int count = 0;
    for(int i=0;i<n;i++) if(s.contains(i)) count++;
    printf("Reader test: %d/%d elements remaining\n", count, n);
}

void run_set_combined_test_impl(int producers, int consumers, int n) {
    FineSet s;
    std::vector<std::atomic<int>> counter(n);
    for(int i=0; i<n; i++) counter[i] = 0;

    // Producers 
    pthread_t p_threads[producers];
    ThreadArg p_args[producers];
    int p_chunk = n / producers;

    for(int i=0; i<producers; i++) {
        int start = i*p_chunk;
        int end = (i==producers-1)? n : (i+1)*p_chunk;
        p_args[i] = {&s, start, end, nullptr};
        pthread_create(&p_threads[i], nullptr, writer_thread, &p_args[i]);
    }
    for(int i=0; i<producers; i++) pthread_join(p_threads[i], nullptr);

    // Consumers 
    pthread_t c_threads[consumers];
    ThreadArg c_args[consumers];
    int c_chunk = n / consumers;

    for(int i=0; i<consumers; i++) {
        int start = i*c_chunk;
        int end = (i==consumers-1)? n : (i+1)*c_chunk;
        c_args[i] = {&s, start, end, counter.data()};
        pthread_create(&c_threads[i], nullptr, reader_thread, &c_args[i]);
    }
    for(int i=0; i<consumers; i++) pthread_join(c_threads[i], nullptr);

    // Check results 
    int missing = 0;
    for(int i=0; i<n; i++) if(counter[i].load() != 1) missing++;
    printf("Combined test: %d/%d elements correctly read\n", n-missing, n);
}

#endif