#include"CDBCUtils.h"
#include"../timer/timer.h"



string CDBCUtils::userName = "xxxx";
string CDBCUtils::password = "xxxx";
string CDBCUtils::url = "127.0.0.1";
string CDBCUtils::db = "webServer";
string CDBCUtils::port = "3306";

CDBCUtils::CDBCUtils() {
    curConnCount = 0;
    freeConnCount = 0;
    //init(10);
}

CDBCUtils* CDBCUtils::getInstance() {
    static CDBCUtils instance;
    return &instance;
}


void CDBCUtils::init(int maxConnCnt = 10) {
    this->maxConnCount = maxConnCnt;
    //MYSQL* mysql = mysql_init(NULL);
    /*
    if (!mysql) {
        Utils::error_handling("mysql_init error!");
    }
    */
   Locker lock;
    for (int i = 0 ; i < maxConnCnt ; i++) {
        //std::cout << "i " << i << std::endl;
        MYSQL *conn = NULL;
        conn = mysql_init(conn);
        if (!conn) {
            Utils::error_handling("mysql_init error!");
        }
        
        conn = mysql_real_connect(conn, url.c_str(), userName.c_str(), password.c_str(), db.c_str(), stoi(port), nullptr, 0);
        if (!conn) {
            Utils::error_handling("mysql_real_connect() error");
        }
        //lock.lock();
        conns.push_back(conn);
        freeConnCount++;
        //lock.unlock();
    }
    semQueue = Sem(freeConnCount);
}



MYSQL* CDBCUtils::getConnection() {
    MYSQL *con = nullptr;
    if (conns.size() == 0) {
        return nullptr;
    }
    semQueue.wait();
    lock.lock();
    con = conns.back();
    conns.pop_back();
    --freeConnCount;
    ++curConnCount;
    lock.unlock();
    return con;
}


bool CDBCUtils::releaseConn(MYSQL *conn) {
    if (!conn) {
        return false;
    }
    lock.lock();
    conns.push_back(conn);
    freeConnCount++;
    curConnCount--;
    semQueue.post();
    lock.unlock();
    return true;
}


int CDBCUtils::getFreConnCount() {
    return freeConnCount;
}

void CDBCUtils::closeResource() {
    lock.lock();
    if (!conns.empty()) {
        auto it = conns.begin();
        while (it != conns.end()) {
            mysql_close(*it);
            it++;
        }
        conns.clear();
        freeConnCount = 0;
        curConnCount = 0;
        conns.clear();
    }
    lock.unlock();
}

CDBCUtils::~CDBCUtils() {
    closeResource();
}

connectionRAII::connectionRAII(MYSQL **conn, CDBCUtils *instance) {
    *conn = instance->getConnection();
    connRAII = *conn;
    poolInstanceRAII = instance;
}

connectionRAII::~connectionRAII() {
    poolInstanceRAII->releaseConn(connRAII);
}

