#include"CDBCUtils.h"
#include<iostream>
int main() {
    MYSQL *conn = nullptr;
    CDBCUtils *pool = CDBCUtils::getInstance();
    pool->init(10);
    connectionRAII mysqlConnRAII(&conn, pool);
    if (mysql_query(conn, "select *from user")) {
        printf("error");
    }
    MYSQL_RES *reuslt = mysql_store_result(conn);
    int numFields = mysql_num_fields(reuslt);
    MYSQL_FIELD *fields = mysql_fetch_field(reuslt);
    while (MYSQL_ROW row = mysql_fetch_row(reuslt)) {
        std::cout << string(row[0]) << "  " << string(row[1]) << std::endl;
    } 
    return 0;
}