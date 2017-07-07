/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: server.cpp
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#include "server.h"
#include <iostream>
#include "product_table_handler.h"
#include "instrument_table_handler.h"
#include "calendar_table_handler.h"
#include "insmrgnrate_table_handler.h"
#include "dcepair_table_handler.h"
#include "wait.h"
#include "project_server.h"
namespace db_sync
{
void server::run()
{
    // 注册请求处理
    std::cout << "server starting" << std::endl;
    init();
    exec();
    //std::cout << "server ending" << std::endl;
}

void server::join()
{
    while (1) {
        if (base::util::local_time_string() ==
            get_project_server().get_process_param().sync_time_) {
            exec();
        }
        sleep_cross(100);
    }
}

void server::stop()
{
    table_handlers_.clear();
}

void server::init()
{
    table_handlers_.push_back( make_shared<product_table_handler>() );
    table_handlers_.push_back( make_shared<instrument_table_handler>() );
    table_handlers_.push_back( make_shared<calendar_table_handler>() );
    table_handlers_.push_back( make_shared<insmrgnrate_table_handler>() );
    table_handlers_.push_back( make_shared<dcepair_table_handler>() );
}

void server::exec()
{
    for (auto it = table_handlers_.begin(); it != table_handlers_.end(); it++) {
        (*it).get()->oracle_to_mysql();
    }
}

}
