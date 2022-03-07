#include"http_conn.h"
#include<string>
#include<fstream>
#include<unordered_map>
#include<iostream>
#include"timer/timer.h"
//#include<string>
#include<unistd.h>
#include<sys/syscall.h>
#include"DBConnection/CDBCUtils.h"

using std::string;
using std::ifstream;
using std::ofstream;
using std::unordered_map;
using std::ios;
const string ok_200_title = "OK";
const string error_400_title = "Dad request";
const string error_400_form = "Your request has bad syntax or is inherently impossible to satisfy.\n";
const string error_403_title = "Forbidden";
const string error_403_form = "You do not have permission to get file from this server.\n";
const string error_404_title = "Not Found";
const string error_404_form = "The requested file was not found on this server.\n";
const string error_500_title = "Internal Error";
const string error_500_form = "There was an unusual problem serving the requested file.\n";
const string error_regist_repeat_title = "The username is repeated";
const string error_regist_repeat_form = "";
const string doc_root = "/home/ubuntu/tcpip/LinuxServer/static";
extern const int TIMESLOT;



int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;
unordered_map<string, string> http_conn::users = {};
//CDBCUtils* http_conn::cdbcUtils = nullptr;

void http_conn::close_conn(bool real_close) {
    if (real_close && (m_sockfd != -1)) {
        Utils::removefd(m_epollfd, m_sockfd);
        close(m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

CDBCUtils* http_conn::initMysql(CDBCUtils* cdbcUtils) {
    initDBByMYSQL(cdbcUtils);
}

void http_conn::init(int sockfd, const sockaddr_in& adr) {
    m_sockfd = sockfd;
    m_addr = adr;
    char ip[100];
    inet_ntop(AF_INET, &(m_addr.sin_addr), ip, INET_ADDRSTRLEN);
    std::cout <<"accept ip: " << ip << std::endl;
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    Utils::addfd(m_epollfd, m_sockfd, true);
    m_user_count++;
    init();
}

void http_conn::init() {
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;
    m_method = GET;
    m_url = "";
    m_version = "";
    m_content_length = 0;
    m_host = "";
    m_start_line = 0;
    m_checked_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    regist_status = NO_REGIST;
    login_status = NO_LOGIN;
    bytes_to_send = 0;
    bytes_have_send = 0;
    this->weakTimePtr = 0;
    //initDBByMYSQL();
    memset(m_read_buf, '\0', READ_BUFFER_SIZE);
    memset(m_write_buf, '\0', WRITE_BUFFER_SIZE);
    memset(m_real_file, '\0', FILENAME_LEN);
}

http_conn::LINE_STATUS http_conn::parse_line() {
    char temp;
    for ( ; m_checked_idx < m_read_idx ; m_checked_idx++) {
        temp = m_read_buf[m_checked_idx];
        if (temp == '\r') {
            if (m_checked_idx + 1 == m_read_idx) {
                return LINE_OPEN;
            }
            else if (m_read_buf[m_checked_idx + 1] == '\n') {
                m_read_buf[m_checked_idx++] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n') {
            if ((m_checked_idx > 1) && (m_read_buf[m_checked_idx - 1] == '\r')) {
                m_read_buf[m_checked_idx - 1] = '\0';
                m_read_buf[m_checked_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

bool http_conn::read() {
    if (m_read_idx == READ_BUFFER_SIZE) {
        puts("read buffer is full\n");
        return false;
    }
    while (true) {
        int len = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
        //std::cout << "len " <<  len << std::endl;
        if (len == -1) {
            if (errno = EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            return false;
        }
        else if (len == 0) {
            return false;
        }
        else {
            m_read_idx += len;
            if (weakTimePtr) {
                weakTimePtr->update(10);
            }
            
        }
    }
    return true;
    //printf("recv %s \n", m_read_buf);
}

bool http_conn::read_lt() {
    int len = recv(m_sockfd, m_read_buf + m_read_idx, READ_BUFFER_SIZE - m_read_idx, 0);
    if (len <= 0) {
        return false;
    }
    m_read_idx += len;
}


//GET http://127.0.0.1/var/www/html/index.html HTTP/1.1
http_conn::HTTP_CODE http_conn::parse_request_line(char *text) {
    char *url = strpbrk(text, " \t");
    if (!url) {
        return BAD_REQUEST;
    }
    *url++ = '\0';
    char *method = text;
    if (strcasecmp(method, "GET") == 0) {
        m_method = GET;
    }
    else if (strcasecmp(method, "POST") == 0) {
        m_method = POST;
    }
    else {
        return BAD_REQUEST;
    }
    url += strspn(url, " \t");
    char *version = strpbrk(url, " \t");
    if (!version) {
        return BAD_REQUEST;
    }
    *version++ = '\0';
    version += strspn(version, " \t");
    if (strcasecmp(version, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }
    if (strncasecmp(url, "http://", 7) == 0) {
        url += 7;
        url = strchr(url, '/');
    }

    if (strncasecmp(url, "https://", 8) == 0) {
        url += 8;
        url = strchr(url, '/');
    }

    if (!url || url[0] != '/') {
        return BAD_REQUEST;
    }
    m_version = version;
    m_url = url;
    //printf("m_version: %s \n", m_version.c_str());
    //printf("m_method: %d \n", m_method);
    //printf("m_url: %s \n", m_url.c_str());
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}



http_conn::HTTP_CODE http_conn::parse_headers(char *text) {
    if (text[0] == '\0') {
        if (m_content_length != 0) {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            m_linger = true;
        }
    }
    else if (strncasecmp(text, "Content-Length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atoi(text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    }
    else {
        //printf("oop! unknow header %s", text);
    }
    return NO_REQUEST;
}

http_conn::HTTP_CODE  http_conn::parse_content(char *text) {
    if (m_read_idx >= (m_content_length + m_checked_idx)) {
        text[m_content_length] = '\0';
        m_content = string(text);
        printf("content: %s \n", text);
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::process_read() {
    LINE_STATUS line_status = LINE_OK;
    m_start_line =  m_checked_idx;
    char *text = nullptr;
    HTTP_CODE ret = NO_REQUEST;
    while (((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK)) || (line_status = parse_line()) == LINE_OK) {
        text = get_line();
        m_start_line = m_checked_idx;
        //printf("got 1 http line: %s \n", text);
        switch (m_check_state) {
            case CHECK_STATE_REQUESTLINE: {
                ret = parse_request_line(text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                break;
            }

            case CHECK_STATE_HEADER: {
                ret = parse_headers(text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                else if (ret == GET_REQUEST) {
                    return do_request();
                }
                break;
            }

            case CHECK_STATE_CONTENT: {
                ret = parse_content(text);
                if (ret == GET_REQUEST) {
                    return do_request();
                }
                line_status = LINE_OPEN;
                break;
            }

            default: {
                return INTERNAL_ERROR;
            }
        }
    }

    return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::do_request() {
    //std::cout << "------------" << std::endl;
    strcpy(m_real_file, doc_root.c_str());
    int len = doc_root.size();
    if (m_url == "/regist") {
        m_url = "/register.html";
    }
    else if (m_url == "/login") {
        m_url = "/login.html";
    }
    else if (m_url == "/insert") {
        registByMysql();
        if (regist_status == REGIST_REPEAT) {
            m_url = "/registError.html";
        }
        else if (regist_status == REGIST_SUCCESS) {
            m_url = "/login.html";
        }
    }
    else if (m_url == "/log") {
        loginByFileSys();
        if (login_status == LOGFIN_FAIL) {
            m_url = "/logError.html";
        }
        else if (login_status == LOGIN_SUCCESS) {
            m_url = "/welcome.html";
        }
    }
    else if (m_url == "/img") {
        m_url = "/picture.html";
    }
    else if (m_url == "/video") {
        m_url = "/video.html";
    }

    //printf("m_url: %s \n", m_url.c_str());

    

    strncpy(m_real_file + len, m_url.c_str(), FILENAME_LEN - len - 1);
    printf("file-path: %s \n", m_real_file);
    
    if (stat(m_real_file, &m_file_stat) < 0) {
        puts("No Resource!");
        return NO_RESOURCE;
    }

    if (!(m_file_stat.st_mode & S_IROTH)) {
        return FORBIDDEN_REQUEST;
    }

    if (S_ISDIR(m_file_stat.st_mode)) {
        return BAD_REQUEST;
    }

    int fd = open(m_real_file, O_RDONLY);

    m_file_address = (char*)mmap(0, m_file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    return FILE_REQUEST;
}

void http_conn::unmap() {
    //std::cout << " unmap " << std::endl;
    if (m_file_address) {
        munmap(m_file_address, m_file_stat.st_size);
        m_file_address = 0;
    }
}

bool http_conn::write() {
    int temp = 0;
    //int bytes_to_send = m_write_idx + m_file_stat.st_size;
    if (bytes_to_send == 0) {
        Utils::modfd(m_epollfd, m_sockfd, EPOLLIN);
        init();
        return true;
    }
    int cnt = 0;
    //std::cout << "write() m_sockfd " << m_sockfd << std::endl;
    while (1) {
        temp = writev(m_sockfd, m_iv, m_iv_count);
       // std::cout << "temp " << temp << std::endl;
        if (temp <= -1) {
            if (errno == EAGAIN) {
                //std::cout << "ERROR-AGAIN" << std::endl;
                //std::cout << "m_epollfd " << m_epollfd << " m_sockfd " << m_sockfd << std::endl;
                Utils::modfd(m_epollfd, m_sockfd, EPOLLOUT);
                return true;
            }
            //std::cout << "errno " << errno << "  m_epollfd " << m_epollfd << " m_sockfd " << m_sockfd << std::endl;
            unmap();
            return false;
        }
        bytes_to_send -= temp;
        bytes_have_send += temp;
        //std::cout << " cnt " << cnt++ << " bytes_to_send " << bytes_to_send << "  temp  "<< temp << " bytes_have_send " << bytes_have_send << std::endl;
        if (bytes_have_send >= m_iv[0].iov_len) {
            m_iv[0].iov_len = 0;
            m_iv[1].iov_base = m_file_address + (bytes_have_send - m_write_idx);
            m_iv[1].iov_len = bytes_to_send;
        }
        else {
            m_iv[0].iov_len = m_iv[0].iov_len - bytes_have_send;
            m_iv[0].iov_base = m_write_buf + bytes_have_send;
        }
        //std::cout << "end " << std::endl;

        if (bytes_to_send <= 0) {
            unmap();
            //m_linger = false;
            fflush(fdopen(m_sockfd, "w"));
            if (m_linger) {
                init();
                std::cout << "send file completed!" << std::endl;
                //close(m_sockfd);
                //shutdown(dup(m_sockfd), SHUTW)
                Utils::modfd(m_epollfd, m_sockfd, EPOLLIN);
                return true;
            }
            else {
                Utils::modfd(m_epollfd, m_sockfd, EPOLLIN);
                return false;
            }
        }
    }
}

bool http_conn::add_response(const char *format, ...) {
    if (m_write_idx >= WRITE_BUFFER_SIZE) {
        return false;
    }
    //printf("format: %s \n", format);
    va_list arg_list;
    va_start(arg_list, format);
    int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE - 1 - m_write_idx, format, arg_list);
    if (len >= WRITE_BUFFER_SIZE - 1 - m_write_idx) {
        return false;
    }
    m_write_idx += len;
    va_end(arg_list);
    return true;
}

bool http_conn::add_status_line(int status, const char *title) {
    return add_response("%s %d %s \r\n", "HTTP/1.1", status, title);
}

bool http_conn::add_headers(int content_len) {
    return add_content_length(content_len) && add_linger() && add_blank_line();
}

bool http_conn::add_content_length(int content_len) {
    return add_response("Content-Length: %d \r\n", content_len);
}

bool http_conn::add_linger() {
    return add_response("Connection: %s\r\n", (m_linger == true) ? "keep-alive" : "close");
}

bool http_conn::add_blank_line() {
    return add_response("%s", "\r\n");
}

bool http_conn::add_content(const char* content) {
    return add_response("%s", content);
}

bool http_conn::process_write(HTTP_CODE ret) {
    switch (ret) {
        case INTERNAL_ERROR: {
            add_status_line(500, error_500_title.c_str());
            add_headers(error_500_form.size());
            if (!add_content(error_500_form.c_str())) {
                return false;
            }
            break;
        }

        case BAD_REQUEST: {
            add_status_line(400, error_400_title.c_str());
            add_headers(error_400_form.size());
            if (!add_content(error_400_form.c_str())) {
                return false;
            }
            break;
        }

        case NO_RESOURCE: {
            add_status_line(404, error_404_title.c_str());
            add_headers(error_404_form.size());
            if (!add_content(error_404_form.c_str())) {
                return false;
            }
            break;
        }

        case FORBIDDEN_REQUEST: {
            add_status_line(403, error_403_title.c_str());
            add_headers(error_403_form.size());
            if (!add_content(error_403_form.c_str())) {
                return false;
            }
            break;
        }

         case FILE_REQUEST: {
            add_status_line(200, ok_200_title.c_str());
            if (m_file_stat.st_size != 0) {
                //std::cout << "m_file_stat.st_size " << m_file_stat.st_size << std::endl;
                bytes_to_send = m_write_idx + m_file_stat.st_size;
                add_headers(m_file_stat.st_size);
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
                return true;
            }
            else {
                const string ok_string = "<html><body></body></html>";
                add_headers(ok_string.size());
                if (! add_content(ok_string.c_str())) {
                    return false;
                }
            }
            
        }
        default: {
            return false;
        }
    }
    
    m_iv[0].iov_len = m_write_idx;
    m_iv[0].iov_base = m_write_buf;
    m_iv_count = 1;
    return true;
}

void http_conn::process() {
    std::cout << "current thread id " << syscall(SYS_gettid) << std::endl; 
    HTTP_CODE read_ret = process_read();
    if (read_ret == NO_REQUEST) {
        Utils::modfd(m_epollfd, m_sockfd, EPOLLIN);
        return;
    }
    bool write_ret = process_write(read_ret);
    if (!write_ret) {
        close_conn();
        this->weakTimePtr->setDeleted();
    }
    Utils::modfd(m_epollfd, m_sockfd, EPOLLOUT);
}

void http_conn::initdbByFileSys() {
    ifstream in("static/db.txt");
    string line;
    //unordered_map<string, string>& users = getusers();
    while (getline(in, line)) {
        //std::cout << "line " << line << std::endl;
        size_t space_pos = line.find(" ");
        string uname = line.substr(0, space_pos);
        string pwd = line.substr(space_pos + 1, line.size() - space_pos - 1);
        //printf("username: %s, password: %s \n", uname.c_str(), pwd.c_str());
        users.emplace(uname, pwd);
    }
}

void http_conn::initDBByMYSQL(CDBCUtils* cdbcUtils) {
    //CDBCUtils* cdbcUtils = CDBCUtils::getInstance();
    if (!cdbcUtils) {
        Utils::error_handling("initDBByMYSQL cdbcUtils error");
    }
    std::cout <<"cdbcUtils address: " << &cdbcUtils << std::endl; 
    MYSQL* conn = nullptr;
    connectionRAII mysqlConnRAII(&conn, cdbcUtils);
    if (!conn) {
        Utils::error_handling("initDBByMYSQL conn error");
    }
    if (mysql_query(conn, "select *from user")) {
        Utils::error_handling("select * from user error");
    }
    MYSQL_RES *result = mysql_store_result(conn);
    if (!result) {
        Utils::error_handling("result error");
    }
    int numFields = mysql_num_fields(result);
    MYSQL_FIELD *fields = mysql_fetch_field(result);
    if (!fields) {
        Utils::error_handling("fields error");
    }
    int i = 0;
    while (MYSQL_ROW row = mysql_fetch_row(result)) {
        //std::cout << string(row[0]) << "  " << string(row[1]) << std::endl;
        string username = row[0];
        string pwd = row[1];
        users[username] = pwd;
        i++;
    } 
    std::cout << "read " << i << " user records " << std::endl;
}




void http_conn::registByFileSys() {
    size_t split_pos = m_content.find("&");
    string uname = m_content.substr(m_content.find("=") + 1, split_pos - 1 - m_content.find("="));
    string pwd = m_content.substr(m_content.rfind("=") + 1, m_content.size() - m_content.rfind("=") - 1);
    if (uname.find(" ") != string::npos || pwd.find(" ") != string::npos) {
        regist_status = REGIST_EMPTY;
        return;
    }
    if (users.find(uname) != users.end()) {
        regist_status = REGIST_REPEAT;
        return;
    }
    regist_status = REGIST_SUCCESS;
    ofstream os("static/db.txt", ios::app | ios::out);
    os << uname << " " << pwd << std::endl;
    //printf("uname: %s, pwd: %s \n", uname.c_str(), pwd.c_str());
    os.close();
}

void http_conn::registByMysql() {
    size_t split_pos = m_content.find("&");
    string uname = m_content.substr(m_content.find("=") + 1, split_pos - 1 - m_content.find("="));
    string pwd = m_content.substr(m_content.rfind("=") + 1, m_content.size() - m_content.rfind("=") - 1);
    if (uname.find(" ") != string::npos || pwd.find(" ") != string::npos) {
        regist_status = REGIST_EMPTY;
        return;
    }
    if (users.find(uname) != users.end()) {
        regist_status = REGIST_REPEAT;
        return;
    }
    else {
        string sql = "insert into user(username, password)values(";
        sql = sql +  "'" + uname + "','" + pwd + "')" ;
        MYSQL* conn = nullptr;
        CDBCUtils *cdbcUtils = CDBCUtils::getInstance();
        connectionRAII mysqlConnRAII(&conn, cdbcUtils);
        lock.lock();
        if (mysql_query(conn, sql.c_str())) {
            Utils::error_handling(sql.c_str());
        }
        users[uname] = pwd;
        regist_status = REGIST_SUCCESS;
        lock.unlock();
    }
}

void http_conn::loginByFileSys() {
    //username=323&password=323
    size_t beg = m_content.find("=") + 1;
    size_t end = m_content.find("&") - 1;
    string uname = m_content.substr(beg, end - beg + 1);
    beg = m_content.rfind("=") + 1;
    end = m_content.size();
    string pwd = m_content.substr(beg, end - beg);
    //std::cout << "uname " << uname << " pwd " << pwd << std::endl;
    if (!users.count(uname) || users[uname] != pwd) {
        /*
        if (!users.count(uname)) {
            std::cout << "can not found uname " << std::endl;
        }
        if (users[uname] != pwd) {
            std::cout << pwd << " password not match " << std::endl;
        }
        */
        login_status = LOGFIN_FAIL;
        return;
    }
    login_status = LOGIN_SUCCESS;
}

/*
void http_conn::loginByMySQL() {

}
*/


