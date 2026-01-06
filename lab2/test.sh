#!/bin/bash

BUILD_DIR="./build"
NUM_CLIENTS=3

# 1. Build project
if [ ! -d "$BUILD_DIR" ]; then
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR" || exit
    cmake ..
    make -j$(nproc)
    cd ..
else
    cd "$BUILD_DIR" || exit
    make -j$(nproc)
    cd ..
fi

echo ""
echo "=============================="
echo "Running tests for all IPCs"
echo "=============================="
echo ""

run_test() {
    local IPC=$1
    echo "------------------------------"
    echo "Testing $IPC IPC"
    echo "------------------------------"

    ./build/host_conn_${IPC} $IPC > host_${IPC}.log 2>&1 &
    HOST_PID=$!
    sleep 1

    for ((i=1; i<=NUM_CLIENTS; i++)); do
        ./build/client_conn_${IPC} $IPC > client_${IPC}_${i}.log 2>&1 &
        CLIENT_PIDS[$i]=$!
        sleep 0.5
    done

    echo "Running for 10 seconds..."
    sleep 10

    echo "Stopping..."
    kill $HOST_PID >/dev/null 2>&1
    for pid in "${CLIENT_PIDS[@]}"; do
        kill "$pid" >/dev/null 2>&1
    done

    echo "===== ${IPC} HOST LOG (last lines) ====="
    tail -n 10 host_${IPC}.log
    echo "===== ${IPC} CLIENT LOG #1 ====="
    tail -n 5 client_${IPC}_1.log
    echo ""
}

run_test "mq"
run_test "fifo"
run_test "sock"

echo "All tests finished. Check log files (host_*.log / client_*.log)"