/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: server.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/
#ifndef __SERVER_H__
#define __SERVER_H__
#include <string>
#include <vector>
#include <memory>

using namespace std;

namespace db_sync{

class table_handler
{
public:
    virtual ~table_handler(){}
    virtual int oracle_to_mysql(void) = 0;
};

class server
{
public:
    inline server()
    {}

    inline ~server()
    {
        stop();
    }

    // 启动服务
    void run();

    // 等待线程完成
    void join();

    // 停止服务
    void stop();

private:
    void init();
    void exec();

private:
    // db_handler
    vector< shared_ptr<table_handler> > table_handlers_;
};

}
#endif
