#pragma once
#ifndef TIMER_H
#define TIMER_H
#include<unistd.h>
#include<signal.h>
#include<sys/types.h>
#include<sys/epoll.h>
#include<fcntl.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<sys/stat.h>
#include<string.h>
#include<pthread.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/mman.h>
#include<stdarg.h>
#include<errno.h>
#include<sys/wait.h>
#include<sys/uio.h>
#include<time.h>
#include<queue>
#include<memory>
#include<algorithm>
#include"../http_conn.h"
class http_conn;
class Utils {
public:
   Utils() {}
   ~Utils() {}
   static void sig_handler(int sig);
   static int setnonblocking(int fd);
   static void addfd(int epollfd, int fd, bool oneshot = true);
   static void removefd(int epollfd, int fd);
   static void modfd(int epollfd, int fd, int ev);
   static void addSig(int sig, void(handler)(int), bool restart = true);
   static void show_error(int connfd, const string& info);
   static void error_handling(const char *message);
public:
   static int *pipefd;
};

class TimerNode {
public:
   TimerNode(http_conn* requestData, int timeslot);
   ~TimerNode() {
      if (httpData) {
         httpData->close_conn();
      }
   }

   TimerNode(const TimerNode& node);
   
   void update(int timeslot = 5);
   
   time_t getExpireTime() {
       return expire;
   }
   
   bool isValid();
   
   const http_conn* getHttpData() const {
      return httpData;
   }

   bool isDeleted() {
      return deleted;
   }

   void setDeleted() {
      deleted = true;
   }

private:
   time_t expire;
   void (*pcb_func)(const http_conn* httpData); 
   http_conn* httpData;
   bool deleted;
};


struct TimerCmp {
  bool operator()(TimerNode *a,
                  TimerNode *b) const {
    return a->getExpireTime() > b->getExpireTime();
  }
};


class HeapTimer {
typedef std::shared_ptr<TimerNode> pTimerNode;
public:
   HeapTimer() {};
   ~HeapTimer() {};
   void addTimer(http_conn* httpData, int timeslot);
   void tick();
private:
   std::priority_queue<TimerNode*, std::deque<TimerNode*>, TimerCmp> minHeap;
   Locker locker; 
};


#endif