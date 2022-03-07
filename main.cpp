/*
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<stdio.h>
#include<unistd.h>
#include<errno.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<cassert>
#include<sys/epoll.h>
#include<memory>
#include"Locker.h"
#include"threadpool.h"
#include"http_conn.h"
#include<string>
#include<vector>
using std::make_shared;
using std::shared_ptr;
using std::vector;
const int MAX_FD = 65536;
const int MAX_EVENT_NUMBER = 10000;



extern int addfd(int epollfd, int fd, bool one_shot);
extern int removefd(int epollfd, int fd);

using std::string;


void addSig(int sig, void(handler)(int), bool restart = true) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void show_error(int connfd, const string& info) {
    printf("%s", info.c_str());
    send (connfd, info.c_str(), info.size(), 0);
    close(connfd);
}

void error_handling(const char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

*/
#include"server.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("usage: %s <port> \n", argv[0]);
        exit(1);
    }
    
    /*
    addSig(SIGPIPE, SIG_IGN);
    threadpool<http_conn>* pool = NULL;
    try {
        pool = new threadpool<http_conn>;
    }
    catch (...) {
        return 1;
    }

    http_conn *users = new http_conn[MAX_FD];
    //vector<shared_ptr<http_conn>> users(MAX_FD, make_shared<http_conn>());
    //assert(users);
    int user_count = 0;

    int serv_sock, clnt_sock;
    struct sockaddr_in clnt_adr, serv_adr;

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1) {
        error_handling("socket() error");
    }
    struct linger tmp = {1, 0};
    setsockopt(serv_sock, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
        error_handling("bind() error");
    }

    if (listen(serv_sock, 5) == -1) {
        error_handling("listen() error");
    }

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    if (epollfd == -1) {
        error_handling("epoll_create() error");
    }
    addfd(epollfd, serv_sock, false);
    http_conn::m_epollfd = epollfd;

    while (true) {
        int event_counts = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((event_counts < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0 ; i < event_counts ; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == serv_sock) {
                socklen_t clnt_adr_sz = sizeof(clnt_adr);
                clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
                if (clnt_sock == -1) {
                    error_handling("accept error()");
                }
                if (http_conn::m_user_count >= MAX_FD) {
                    show_error(clnt_sock, "Interval server busy");
                    continue;
                }
                users[clnt_sock].init(clnt_sock, clnt_adr);
                
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                users[sockfd].close_conn();
            }
            else if (events[i].events & EPOLLIN) {
                if (users[sockfd].read()) {
                    pool->append(users + sockfd);
                }
                else {
                    users[sockfd].close_conn();
                }
            }
            else if (events[i].events & EPOLLOUT) {
                if (!users[sockfd].write()) {
                    users[sockfd].close_conn();
                }
            }
            else {

            }
        }
    }
    

   
    close(epollfd);
    close(serv_sock);
    delete pool;
    */
   
   
   server myServer;
   myServer.initDataBase();
   myServer.createPool();
   myServer.createSocket(atoi(argv[1]));
   myServer.epollLoop();
   
    
   return 0;
}
