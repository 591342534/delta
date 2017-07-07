/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: project_server.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#ifndef __PROJECT_SERVER_H__
#define __PROJECT_SERVER_H__

#include <iostream>
#include <string>

#include "server.h"
#include "singleton.h"
#include "database/unidbpool.h"
#include "base/log_binder.h"

namespace db_sync {

struct db_info {
    //dbtype eg:mysql
    std::string dbtype;
    database::unidb_param dbparam;
};

struct process_param {
    std::string server_name_;
    std::string product_table_str_;
    std::string instrument_table_str_;
    std::string calendar_table_str_;
    std::string insmrgnrate_table_str_;
    std::string dcepair_inscom_table_str_;
    std::string oracle_product_table_str_;
    std::string oracle_instrument_table_str_;
    std::string oracle_calendar_table_str_;
    std::string oracle_insmrgnrate_table_str_;
    std::string oracle_dcepair_inscom_table_str_;
    std::string sync_time_;
    db_info mysql_dbinfo_;
    db_info oracle_dbinfo_;
    process_param()
            : server_name_(""),
              product_table_str_(""),
              instrument_table_str_(""),
              calendar_table_str_(""),
              insmrgnrate_table_str_(""),
              dcepair_inscom_table_str_(""),
              oracle_product_table_str_(""),
              oracle_instrument_table_str_(""),
              oracle_calendar_table_str_(""),
              oracle_insmrgnrate_table_str_(""),
              oracle_dcepair_inscom_table_str_(""),
              sync_time_("")
    {
    }
};

class project_server {
    SINGLETON_ONCE(project_server);
public:
    void start(std::string &path);
    void join();
    void stop();

    process_param& get_process_param() { return params_; }
    database::db_conn_pool& get_mysql_db_pool() { return mysql_db_pool_; }
    database::db_conn_pool& get_oracle_db_pool() { return oracle_db_pool_; }
private:
    int load_config(const char* config_file);
private:
    process_param params_;
    database::db_conn_pool mysql_db_pool_;
    database::db_conn_pool oracle_db_pool_;
    server m_app_server;
};
SINGLETON_GET(project_server);
}
#endif
