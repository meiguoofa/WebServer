#pragma once
#ifndef HTTPCONNECTION_H
#define HTTPCONNECTION_H

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
#include<sys/uio.h>
#include"Locker.h"
#include<unordered_map>
#include<string>
#include<memory>
#include<iostream>
//#include"./DBConnection/CDBCUtils.h"
using std::unordered_map;
using std::string;
using std::shared_ptr;
using std::weak_ptr;
class TimerNode;
class CDBCUtils;
class http_conn {
public:
     static const int FILENAME_LEN = 200;
     static const int READ_BUFFER_SIZE = 2048;
     static const int WRITE_BUFFER_SIZE = 1024;
     enum METHOD {
         GET = 0, POST, HEAD, PUT, DELETE, TRACE, OPTIONS, CONNECT, PATCH
     };

     enum CHECK_STATE {
         CHECK_STATE_REQUESTLINE = 0,
         CHECK_STATE_HEADER,
         CHECK_STATE_CONTENT
     };

     enum HTTP_CODE {
         NO_REQUEST, GET_REQUEST, BAD_REQUEST, NO_RESOURCE,
         FORBIDDEN_REQUEST, FILE_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION, NO_REGIST,
         REGIST_EMPTY, REGIST_REPEAT, REGIST_SUCCESS, NO_LOGIN, LOGFIN_FAIL, LOGIN_SUCCESS
     };

     enum LINE_STATUS {
         LINE_OK = 0, LINE_BAD, LINE_OPEN
     };


public:
    http_conn() {};
    ~http_conn() {
        //std::cout << "http_conn close " << std::endl;
        /*
        if (weakTimePtr) {
            delete weakTimePtr;
        }
        */
    };

public:
    //初始化新接受的连接
    void init(int sockfd, const sockaddr_in &addr);
    //关闭连接
    void close_conn(bool real_close = true);
    //处理用户请求
    void process();
    //非阻塞读写
    bool read();
    bool read_lt();
    bool write();
    int getSockFd() const {
        return m_sockfd;
    }
    void connectTimer(TimerNode *ptrTimerNode) {
        this->weakTimePtr = ptrTimerNode;
    }

    TimerNode* getTimerNode() {
        return weakTimePtr;
    }

    CDBCUtils* initMysql(CDBCUtils* cdbcUtils);

private:
    //初始化连接
    void init();
    //解析HTTP请求
    HTTP_CODE process_read();
    //填充HTTP应答
    bool process_write(HTTP_CODE ret);
    //initdb();
    void initdbByFileSys();
    //init db by mysql
    void initDBByMYSQL(CDBCUtils* cdbcUtils);

    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers (char *text);
    HTTP_CODE parse_content (char *text);
    HTTP_CODE do_request();
    char* get_line() { return m_read_buf + m_start_line; }
    LINE_STATUS parse_line();

    void unmap();
    bool add_response(const char *format, ...);
    bool add_content(const char *content);
    bool add_status_line (int status, const char *title);
    bool add_headers (int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();
    void registByFileSys();
    void registByMysql();
    void loginByFileSys();
    void loginByMySQL();
    

public:
    //所有socket上的事件都被注册到同一个epoll内核事件表中，所以将epoll文件描述符设置为静态的
    static int m_epollfd;
    //统计用户数量
    static int m_user_count;


private:
    int m_sockfd;
    sockaddr_in m_addr;
    char m_read_buf[READ_BUFFER_SIZE];
    //读缓冲中已经读入的客户数据的最后一个字节的下一个位置
    int m_read_idx;
    //当前正在分析的字符在读缓冲区中的位置
    int m_checked_idx;
    //当前正在解析的行的起始位置
    int m_start_line;
    //写缓冲区
    char m_write_buf[WRITE_BUFFER_SIZE];
    //写缓冲区红待发送的字节数
    int m_write_idx;
    // 主状态机当前所处的状态
    CHECK_STATE m_check_state;
    // 请求方法
    METHOD m_method;
    // 客户请求的目标文件的完整路径，其内容等于doc_root + m_url, doc_root是网站根目录
    char m_real_file[FILENAME_LEN];
    // 客户请求的目标文件的文件名
    string m_url;
    //HTTP协议版本号，
    string m_version;
    // 主机名
    string m_host;
    //内容
    string m_content;
    // HTTP请求的消息体的长度
    int m_content_length;
    // HTTP请求是否要求保持连接
    bool m_linger;
    // 客户请求的目标文件被mmap到内存中的起始位置
    char *m_file_address;
    //目标文件的状态，通过它我们可以判断文件是否存在，是否为目录，是否可读，并获取文件大小等信息
    struct stat m_file_stat;
    // 采用writev来执行写操作
    struct iovec m_iv[2];

    int m_iv_count;

    static unordered_map<string, string> users;

    HTTP_CODE regist_status;

    HTTP_CODE login_status;

    int bytes_to_send;

    int bytes_have_send;

    TimerNode *weakTimePtr;

    //static CDBCUtils *cdbcUtils;

    Locker lock;

    //DataBase
//public:    
    //static unordered_map<string, string> users; 
};

#endif

