#pragma once
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
#include"Locker.h"
#include"threadpool.h"
#include"http_conn.h"
#include"./timer/timer.h"
#include<string>
#include<vector>
//#include"./DBConnection/CDBCUtils.h"
using std::vector;
const int MAX_FD = 65536;
const int MAX_EVENT_NUMBER = 10000;
const int TIMESLOT = 10;
//class Utils;
class CDBCUtils;
class server {
public:
   server();
   ~server();
   int createPool();
   void createSocket(int port);
   void epollLoop();
   void closeAll();
   void initDataBase();

private:
    //part 1
    int m_port;
    int m_pipefd[2];
    int m_epollfd;
    //vector<shared_ptr<http_conn>> users;
    http_conn *users;

    int m_server_sock;
    sockaddr_in serv_adr;

    shared_ptr<threadpool<http_conn>> m_pool;
    int m_thread_num;

    epoll_event events[MAX_EVENT_NUMBER];

    //timer
    HeapTimer heapTimer;

    //DataBase
    CDBCUtils* cdbcUtils;

    Utils utils;
};

