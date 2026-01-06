#include <iostream>
#include <cstdlib>
#include <ctime>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include "conn.h"

#ifdef USE_MQ
#include "conn_mq.h"
#elif defined USE_FIFO
#include "conn_fifo.h"
#elif defined USE_SOCK
#include "conn_sock.h"
#endif

sem_t* sem;

int main(){
    srand(time(0));
    sem = sem_open("/wolf_sem",0);

#ifdef USE_MQ
    Conn* conn = new ConnMQ("/wolf_queue",false);
#elif defined USE_FIFO
    Conn* conn = new ConnFIFO("/tmp/wolf_fifo",false);
#elif defined USE_SOCK
    Conn* conn = new ConnSock("12345",false);
#else
#error "No IPC defined"
#endif

    int goat_status=1;
    while(true){
        int goat_num = goat_status ? rand()%100+1 : rand()%50+1;

        sem_wait(sem);
        conn->Write(&goat_num,sizeof(goat_num));
        sem_post(sem);

        sem_wait(sem);
        conn->Read(&goat_status,sizeof(goat_status));
        sem_post(sem);

        std::cout<<"Goat status: "<<(goat_status?"Alive":"Dead")<<"\n";
        sleep(1);
    }

    delete conn;
    sem_close(sem);
    return 0;
}