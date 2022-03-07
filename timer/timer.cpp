#include"timer.h"
#include<iostream>
//#include"Locker.h"
void cb_func(const http_conn* httpData) {
    if (!httpData) {
        return;
    }
    if (httpData->getSockFd() != -1) {
        epoll_ctl(httpData->m_epollfd, EPOLL_CTL_DEL, httpData->getSockFd(), 0);
        close(httpData->getSockFd());
        printf("close the fd: %d \n", httpData->getSockFd());
    }
}

void TimerNode::update(int timeslot) {
    expire = time(NULL) + 3 * timeslot;
}

TimerNode::TimerNode(http_conn* requestData, int timeslot) : httpData(requestData), deleted(false) {
    this->pcb_func = cb_func;
    this->expire = time(NULL) + 3 * timeslot;
}

TimerNode::TimerNode(const TimerNode& node):httpData(node.httpData), expire(0), deleted(false) {
    this->pcb_func = cb_func;
}

bool TimerNode::isValid() {
    time_t currentTime = time(NULL);
    if (currentTime < expire) {
        return true;
    }
    else {
        this->setDeleted();
        return false;
    }
}


void HeapTimer::addTimer(http_conn* httpData, int timeslot) {
    TimerNode *node = new TimerNode(httpData, timeslot);
    {   
        locker.lock();
        minHeap.push(node);
        httpData->connectTimer(node);
        locker.unlock();
    }
}

void HeapTimer::tick() {
    //std::cout << " tick() " << std::endl;
    while (!minHeap.empty()) {
        TimerNode *ptrTimerNode = minHeap.top();
        if (ptrTimerNode && ptrTimerNode->isDeleted()) {
            minHeap.pop();
            delete ptrTimerNode;
        }
        else if (ptrTimerNode && !ptrTimerNode->isValid()) {
            //std::cout << "socket fd " << ptrTimerNode->getHttpData()->getSockFd() << " is closed " << std::endl;
            cb_func(ptrTimerNode->getHttpData());
            minHeap.pop();
            if (ptrTimerNode) {
                delete ptrTimerNode;
            }
        }
        else {
            break;
        }
    }
}



void Utils::sig_handler(int sig) {
        int save_errno = errno;
        int msg = sig;
        //std::cout << "sig " << sig << " is activated " << std::endl;
        //std::cout << "pipefd[1] " << pipefd[1] << std::endl;
        send(pipefd[1], (void*)&msg, 1, 0);
        errno = save_errno;
}

int Utils::setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, old_option | O_NONBLOCK);
    return old_option;
}

void Utils::addfd(int epollfd, int fd, bool oneshot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if (oneshot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void Utils::removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}

void Utils::modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLRDHUP | EPOLLET | EPOLLONESHOT | ev;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

void Utils::addSig(int sig, void(handler)(int), bool restart) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig, &sa, NULL) != -1);
}

void Utils::show_error(int connfd, const string& info) {
    printf("%s", info.c_str());
    send (connfd, info.c_str(), info.size(), 0);
    close(connfd);
}

void Utils::error_handling(const char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int *Utils::pipefd = 0;