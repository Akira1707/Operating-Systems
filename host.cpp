#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <semaphore.h>
#include <fcntl.h>
#include <unistd.h>

#include "conn.h"

#ifdef USE_MQ
#include "conn_mq.h"
#elif defined USE_FIFO
#include "conn_fifo.h"
#elif defined USE_SOCK
#include "conn_sock.h"
#endif

sem_t* sem;

int main() {
    srand(time(0));

    sem = sem_open("/wolf_sem", O_CREAT, 0666, 1);

    int num_goats;
    std::cout << "Enter number of goats (1-100): ";
    std::cin >> num_goats;

#ifdef USE_MQ
    Conn* conn = new ConnMQ("/wolf_queue", true);
#elif defined USE_FIFO
    Conn* conn = new ConnFIFO("/tmp/wolf_fifo", true);
#elif defined USE_SOCK
    Conn* conn = new ConnSock("12345", true);
#else
#error "No IPC defined"
#endif

    std::vector<int> goat_status(num_goats, 1);
    int dead_prev = 0;

    while(true) {
        int wolf_num;
        std::cout << "\nEnter wolf number (1-100, 3s timeout): ";
        fd_set set; FD_ZERO(&set); FD_SET(0, &set);
        struct timeval tv = {3,0};
        int rv = select(1, &set, NULL, NULL, &tv);
        if(rv>0) std::cin >> wolf_num;
        else wolf_num = rand()%100+1;
        std::cout << "Wolf: " << wolf_num << "\n";

        for(int i=0;i<num_goats;i++){
            int goat_num;
            sem_wait(sem);
            conn->Read(&goat_num, sizeof(goat_num));
            sem_post(sem);

            std::cout << "Goat[" << i << "]: " << goat_num;
            if(goat_status[i]==1){
                if(abs(goat_num-wolf_num)<=70/num_goats) std::cout<<" -> Hid\n";
                else { goat_status[i]=0; std::cout<<" -> Caught\n"; }
            } else {
                if(abs(goat_num-wolf_num)<=20/num_goats){ goat_status[i]=1; std::cout<<" -> Revived\n";}
                else std::cout<<" -> Still dead\n";
            }
        }

        for(int i=0;i<num_goats;i++){
            sem_wait(sem);
            conn->Write(&goat_status[i], sizeof(goat_status[i]));
            sem_post(sem);
        }

        int dead_curr = 0;
        for(auto s:goat_status) if(s==0) dead_curr++;
        std::cout << "Total dead goats: " << dead_curr << "\n";

        if(dead_curr==num_goats && dead_prev==num_goats){
            std::cout << "All goats dead 2 consecutive rounds. Game over!\n";
            break;
        }
        dead_prev = dead_curr;
    }

    delete conn;
    sem_close(sem);
    sem_unlink("/wolf_sem");
    return 0;
}