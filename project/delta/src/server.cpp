/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: server.cpp
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#include "server.h"
#include "project_server.h"
#include "default_handler.h"
#include "qry_instrument_handler.h"
#include "qry_insmrgnrate_handler.h"
#include "qry_dcepair_handler.h"
#include "qry_expireinst_handler.h"
#include "db_deposit_handler.h"
namespace serverframe
{

void server::run(const size_t thread_num)
{
    // 注册请求处理
    std::cout << "server starting" << std::endl;

    message_server_.register_handle(std::bind(&server::init, this, std::placeholders::_1));

    // 开启处理者线程
    message_server_.run(thread_num);

    // tcp连接到消息总线
    int port = get_project_server().get_process_param().serverinfo_.port;
    get_tcp_engine().init("127.0.0.1", port, tcpserver_notify_);

}

void server::join()
{
    message_server_.join();
}

void server::stop()
{
    message_server_.stop();
}

void server::init(MessageDispatcher& dispatcher)
{
    // 默认处理请求.
    dispatcher.route_default<default_handler>();

    // 查询instrument
    dispatcher.route<qry_instrument_handler>(common::TYPE_MANAGERSERVER_INSTRUMENT_REQ);

    // 查询insmrgnrate
    dispatcher.route<qry_insmrgnrate_handler>(common::TYPE_MANAGERSERVER_INSMRGNRATE_REQ);

    // 查询dcepair
    dispatcher.route<qry_dcepair_handler>(common::TYPE_MANAGERSERVER_DCEPAIR_REQ);

    // 查询expireinst
    dispatcher.route<qry_expireinst_handler>(common::TYPE_MANAGERSERVER_EXPIREINST_REQ);

    // deposit to db
    dispatcher.route<db_deposit_handler>(common::TYPE_CUSTOMER_CASH_DEPOSIT_RSP);
}

}// ns::serverframe
