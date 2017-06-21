#ifndef __PROJECT_SERVER_H__
#define __PROJECT_SERVER_H__

#include <iostream>
#include <string>

#include "server.h"
#include "singleton.h"
#include "database/unidbpool.h"
#include "base/log_binder.h"

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
    std::string instrument_table_str_;
    std::string update_data_time_;
    db_info dbinfo_;
    server_info serverinfo_;
    process_param()
            : server_name_("")
            , instrument_table_str_("")
            , update_data_time_("")
    {
    }
};

class project_server: public singleton<project_server> {
private:
    project_server(){}
    virtual ~project_server(){}

    friend class singleton<project_server> ;
public:
    void start(std::string &path);
    void join();
    void stop();

    process_param& get_process_param() { return params_; }
    database::db_conn_pool& get_db_conn_pool() { return db_pool_; }
private:
    int load_config(const char* config_file);
private:
    process_param params_;
    database::db_conn_pool db_pool_;
    server m_app_server;
};

}
#endif
