#include"server.h"
#include"DBConnection/CDBCUtils.h"
#include"iostream"
//#include"time.h"
using std::make_shared;
//extern int addfd(int epollfd, int fd, bool one_shot);
//extern int removefd(int epollfd, int fd);
//extern int setnonblocking(int fd);
//extern int setnonblocking(int fd);
//extern void modfd(int epollfd, int fd, int ev);
void error_handling(const char *message) {
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

/*
int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, old_option | O_NONBLOCK);
    return old_option;
}

void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLRDHUP | EPOLLET | EPOLLONESHOT | ev;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}
*/

/*

int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    fcntl(fd, F_SETFL, old_option | O_NONBLOCK);
    return old_option;
}

void addfd(int epollfd, int fd, bool oneshot = true) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if (oneshot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}

void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLRDHUP | EPOLLET | EPOLLONESHOT | ev;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

*/







server::server() {
    Utils::addSig(SIGPIPE, SIG_IGN);
    users = new http_conn[MAX_FD];
}

void server::initDataBase() {
    this->cdbcUtils = CDBCUtils::getInstance();
    this->cdbcUtils->init(8);
    users->initMysql(cdbcUtils);
}

int server::createPool() {
    try {
        m_pool = std::make_shared<threadpool<http_conn>>();
    }
    catch (...) {
        return 1;
    }
    return 0;
} 

void server::createSocket(int port) {
    struct sockaddr_in serv_adr;
    m_server_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (m_server_sock == -1) {
        error_handling("socket() error");
    }
    struct linger tmp = {1, 0};
    setsockopt(m_server_sock, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(port);
    int flag = 1;
    setsockopt(m_server_sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    if (bind(m_server_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
        error_handling("bind() error");
    }

    if (listen(m_server_sock, 5) == -1) {
        error_handling("listen() error");
    }

    if (socketpair(PF_UNIX, SOCK_STREAM, 0, m_pipefd) == -1) {
        error_handling("socketpair error");
    }
    
    //std::cout << "pipe read " << m_pipefd[0] << " pipe write " << m_pipefd[1] << std::endl;

    utils.pipefd = m_pipefd;

    
    Utils::setnonblocking(m_pipefd[1]);
    Utils::addSig(SIGALRM, Utils::sig_handler, false);
    Utils::addSig(SIGTERM, Utils::sig_handler, false);
}

void server::epollLoop() {
    alarm(TIMESLOT);
    struct sockaddr_in clnt_adr;
    //int clnt_sock;
    int epollfd = epoll_create(5);
    if (epollfd == -1) {
        error_handling("epoll_create() error");
    }
    http_conn::m_epollfd = epollfd;
    Utils::addfd(epollfd, m_pipefd[0], false);
    Utils::addfd(epollfd, m_server_sock, false);
    bool run = true;
    bool timeout = false;
    while (run) {
        int event_counts = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if ((event_counts < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }

        for (int i = 0 ; i < event_counts ; i++) {
            int sockfd = events[i].data.fd;
            //std::cout << "sockfd " << sockfd << std::endl;
            if (sockfd == m_server_sock) {
                socklen_t clnt_adr_sz = sizeof(clnt_adr);
                int clnt_sock = accept(m_server_sock, (struct sockaddr*)&clnt_adr, &clnt_adr_sz);
                if (clnt_sock == -1) {
                    error_handling("accept error()");
                }
                if (http_conn::m_user_count >= MAX_FD) {
                    Utils::show_error(clnt_sock, "Interval server busy");
                    continue;
                }
                users[clnt_sock].init(clnt_sock, clnt_adr);
                heapTimer.addTimer(users + clnt_sock, 5);
            }
            else if (sockfd == m_pipefd[0] && (events[i].events & EPOLLIN)) {
                //std::cout << "received msg from pipe " << std::endl;
                char signal[1024];
                int ret = recv(m_pipefd[0], signal, sizeof(signal), 0);
                if (ret == -1) {
                    continue;
                }
                else if (ret == 0) {
                    continue;
                }
                else {
                    for (int i = 0 ; i < ret ; i++) {
                        switch (signal[i]) {
                            case SIGALRM: {
                                //std::cout << "sig alarm" << std::endl;
                                timeout = true;
                            }
                            case SIGCHLD:
                            case SIGHUP: {
                                continue;
                            }
                            case SIGTERM:
                            case SIGINT: {
                                puts("server terminal\n");
                                run = false;
                            }
                        }
                    }
                }
            }
            else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                std::cout << "events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)" << std::endl;
                users[sockfd].close_conn();
                //users[sockfd].getTimerNode()->setDeleted();
                if (users[sockfd].getTimerNode()) {
                    users[sockfd].getTimerNode()->setDeleted();
                }
            }
            else if (events[i].events & EPOLLIN) {
                if (users[sockfd].read()) {
                    //std::cout << " m_pool->append() 1 " << sockfd <<  std::endl;
                    m_pool->append(users + sockfd);
                    //std::cout << " m_pool->append() 2 " << sockfd <<  std::endl;
                }
                else {
                    //std::cout << "read completed! " << std::endl;
                    users[sockfd].close_conn();
                    if (users[sockfd].getTimerNode()) {
                       users[sockfd].getTimerNode()->setDeleted();
                    }
                }
                //std::cout << "33333333333333" <<std::endl;
            }
            else if (events[i].events & EPOLLOUT) {
                if (!users[sockfd].write()) {
                    //std::cout << "-----close conn ------" << std::endl;
                    //printf("error occun\n");
                    users[sockfd].close_conn();
                    //printf("error 1");
                    if (users[sockfd].getTimerNode()) {
                       users[sockfd].getTimerNode()->setDeleted();
                    }
                    //printf("error2");
                }
                else {
                   // std::cout << "File transmission completed!" << std::endl;
                    //printf("error 3 \n");
                    
                    TimerNode *node = users[sockfd].getTimerNode();
                    if (node) {
                        node->update(10);
                    }
                    
                    //printf("error 4 \n");
                }
            }
            else {

            }
        }
        if (timeout) {
            heapTimer.tick();
            timeout = false;
            alarm(TIMESLOT);
        }
    }
}

void server::closeAll() {
    close(m_epollfd);
    close(m_server_sock);
}

server::~server() {
    closeAll();
}