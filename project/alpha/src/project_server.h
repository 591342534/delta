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
#include <memory>
#include "utility/singleton.h"
#include "database/unidbpool.h"
#include "base/log_binder.h"
#include "query_processor.h"
#include "deals_processor.h"
#include "processor_base.h"
#include "http_processor.h"
#include "http_unit.h"
namespace serverframe {
struct db_info {
    //dbtype eg:mysql
    std::string dbtype;
    database::unidb_param dbparam;
};

struct server_info {
    std::string ip;
    int port;
    server_info()
            : ip("127.0.0.1")
            , port(7779)
    {
    }
};
struct process_param {
    std::string server_name_;
    int query_processor_thread_count_;
    std::string instrument_table_str_;
    std::string insmrgnrate_table_str_;
    std::string dcepair_inscom_table_str_;
    std::string calendar_table_str_;
    std::string calcdeposit_table_str_;
    std::string update_data_time_;
    db_info dbinfo_;
    server_info serverinfo_;
    process_param()
            : server_name_("")
            , query_processor_thread_count_(0)
            , instrument_table_str_("")
            , insmrgnrate_table_str_("")
            , dcepair_inscom_table_str_("")
            , calcdeposit_table_str_("")
            , update_data_time_("")
    {
    }
};

class project_server {
    SINGLETON_ONCE_UNINIT(project_server);
private:
    project_server();

public:
    ~project_server();

public:
    int start(std::string &path);
    int stop();

    int start_internal();
    int stop_internal();

    int dispatch_message(atp_message& msg);
    int get_index(const char* key, int bound);

    process_param& get_process_param() { return params_; }
    database::db_conn_pool& get_db_conn_pool() { return db_pool_; }

private:
    int load_config(const char* config_file);

private:
    process_param params_;
    database::db_conn_pool db_pool_;

    deals_processor* m_deal_pro_;
    std::vector<query_processor*> m_query_pro_vec_;
    std::shared_ptr<http_processor> m_http_pro_;
    std::shared_ptr<http_unit> m_http_unit_sptr_;
    bool started_;
};
SINGLETON_GET(project_server);
}
#endif
