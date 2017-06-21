#pragma once

#ifndef __mysql_wrapper_H__ 
#define __mysql_wrapper_H__ 

#ifdef _WIN32
#include <winsock.h>
#endif 

#include <iostream> 
#include <string> 
#include <vector> 
#include <string> 
#include <string.h>
#include <stdio.h>
  
//#include "fspcommon.h"
#include "utility/utility.h"
#include "mysql/include/mysql.h" 


namespace mysql_wrapper {

    class COMMON_API mysql_wrapper
    {
    public:
        mysql_wrapper();
        virtual ~mysql_wrapper();

        static mysql_wrapper * get_instance();
        bool connet(char* server, char* username, char* password, char* database, int port);
        void close();

        bool create_database(std::string& dbname);
        bool create_table(const std::string& query);

        bool execute_sql(std::string strSql);
        bool fetch_data(std::string queryStr, std::vector<std::vector<std::string> >& data);
        int affected_rows();

        void run_failed();
        void get_last_error(int &nErrorCode, std::string &errorMsg);

    private:
        int error_num;                       //错误代号 
        const char* error_info;              //错误提示 

        MYSQL mysql_instance;                //MySQL对象，必备的一个数据结构 
        MYSQL_RES *result;                  //用于存放结果 建议用char* 数组将此结果转存 

        static mysql_wrapper *instance;
        char _server[16];
        char _username[32];
        char _password[32];
        char _database[32];
        int _port;
    };

}

#endif

