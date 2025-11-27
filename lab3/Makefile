CXX = g++
CXXFLAGS = -O2 -std=gnu++11 -pthread
TARGET = lab3

SRCS = main.cpp \
       sets/coarse_set.h sets/fine_set.h sets/node.h \
       queues/scsp_queue.h queues/mcsp_queue.h \
       tests/test_sets.h tests/test_queues.h tests/benchmark.h

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) main.cpp -o $(TARGET)

clean:
	rm -f $(TARGET)
