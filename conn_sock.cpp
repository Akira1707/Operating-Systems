#include "conn_sock.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sys/select.h>

ConnSock::ConnSock(const std::string &port, bool create) : Conn("", create) {
    int p = std::stoi(port);
    if(create){
        int server_fd = socket(AF_INET, SOCK_STREAM,0);
        if(server_fd<0){ perror("socket"); exit(1);}
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(p);
        int opt=1;
        setsockopt(server_fd,SOL_SOCKET,SO_REUSEADDR|SO_REUSEPORT,&opt,sizeof(opt));
        if(bind(server_fd,(sockaddr*)&addr,sizeof(addr))<0){ perror("bind"); exit(1);}
        listen(server_fd,100); 
        socklen_t len = sizeof(addr);
        sockfd = accept(server_fd,(sockaddr*)&addr,&len);
        if(sockfd<0){ perror("accept"); exit(1);}
        close(server_fd);
    } else {
        sockfd = socket(AF_INET, SOCK_STREAM,0);
        if(sockfd<0){ perror("socket"); exit(1);}
        sockaddr_in serv{};
        serv.sin_family = AF_INET;
        serv.sin_port = htons(p);
        serv.sin_addr.s_addr = inet_addr("127.0.0.1");
        while(connect(sockfd,(sockaddr*)&serv,sizeof(serv))<0) sleep(1);
    }
}

bool ConnSock::Read(void *buf, size_t count) {
    fd_set set; FD_ZERO(&set); FD_SET(sockfd,&set);
    struct timeval tv={5,0};
    int rv = select(sockfd+1,&set,NULL,NULL,&tv);
    if(rv>0){
        ssize_t r = read(sockfd,buf,count);
        return r == (ssize_t)count;
    }
    std::cerr<<"Socket read timeout\n";
    return false;
}

bool ConnSock::Write(void *buf, size_t count) {
    ssize_t r = write(sockfd,buf,count);
    return r == (ssize_t)count;
}

ConnSock::~ConnSock() { close(sockfd); }