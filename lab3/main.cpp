// main.cpp
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "tests/test_sets.h"
#include "tests/test_queues.h"
#include "tests/benchmark.h"

int main(int argc, char** argv) {
    int producers = 1;
    int consumers = 4;
    int n = 10000;
    size_t capacity = 1 << 16;
    int runs = 3;
    const char* test = "all";

    // Parse command-line arguments 
    for (int i=1; i<argc; i++) {
        if (strcmp(argv[i], "--producers")==0 && i+1<argc) producers = atoi(argv[++i]);
        else if (strcmp(argv[i], "--consumers")==0 && i+1<argc) consumers = atoi(argv[++i]);
        else if (strcmp(argv[i], "--n")==0 && i+1<argc) n = atoi(argv[++i]);
        else if (strcmp(argv[i], "--cap")==0 && i+1<argc) capacity = (size_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--runs")==0 && i+1<argc) runs = atoi(argv[++i]);
        else if (strcmp(argv[i], "--test")==0 && i+1<argc) test = argv[++i];
        else if (strcmp(argv[i], "--help")==0) {
            printf("Usage: %s [--test <all|set_all|queue_all|set_bench|queue_bench>] [--producers P] [--consumers C] [--n N] [--cap CAP] [--runs R]\n", argv[0]);
            return 0;
        }
    }

    // TESTS
    if (strcmp(test, "all")==0 || strcmp(test, "set_all")==0) {
        printf("===== Running Set Tests =====\n");
        run_set_writer_test_impl(producers, n);
        run_set_reader_test_impl(consumers, n);
        run_set_combined_test_impl(producers, consumers, n);
    }

    if (strcmp(test, "all")==0 || strcmp(test, "queue_all")==0) {
        printf("===== Running Queue Tests =====\n");
        if(producers > 1) {
            printf("MCSPQueue supports only 1 producer, forcing producers=1\n");
            producers = 1;
        }
        run_queue_combined_test_impl(1, capacity, producers, consumers, n);
    }

    // BENCHMARKS
    if (strcmp(test, "all")==0 || strcmp(test, "set_bench")==0) {
        printf("\n===== Benchmark FineSet Multi-threaded =====\n");
        printf("\n--- Randomized ---\n");
        benchmark_sets_mt(producers, n, runs, true);
        printf("\n--- Fixed ---\n");
        benchmark_sets_mt(producers, n, runs, false);
    }

    if (strcmp(test, "all")==0 || strcmp(test, "queue_bench")==0) {
        printf("\n===== Benchmark MCSPQueue Multi-threaded =====\n");
        if(producers > 1) producers = 1; 
        printf("\n--- Randomized ---\n");
        benchmark_mcsp_queue(producers, capacity, n, runs, true);
        printf("\n--- Fixed ---\n");
        benchmark_mcsp_queue(producers, capacity, n, runs, false);
    }

    return 0;
}