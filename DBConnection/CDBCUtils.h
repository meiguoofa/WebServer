#include<string>
using std::string;
#include<mysql/mysql.h>
#include"../Locker.h"
#include<list>
#include<vector>
class CDBCUtils;
class CDBCUtils {
public:
   static CDBCUtils* getInstance();
   MYSQL* getConnection();
   bool releaseConn(MYSQL *conn);
   int getFreConnCount();
   void closeResource();
   void init(int maxConnCnt);

private:
   CDBCUtils();
   ~CDBCUtils();
   
private:
   static string userName;
   static string password;
   static string url;
   static string db;
   static string port;
   //static CDBCUtils* instance;

private:
   int maxConnCount;
   int curConnCount;
   int freeConnCount;
   Locker lock;
   std::vector<MYSQL*> conns;
   Sem semQueue;
};

class connectionRAII {
public:
    connectionRAII(MYSQL **conn, CDBCUtils *instance);
    ~connectionRAII();

private:
    MYSQL *connRAII;
    CDBCUtils *poolInstanceRAII;
};