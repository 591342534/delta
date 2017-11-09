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
#include "tcp_server_notify.h"
#include "tcp_engine.h"

namespace serverframe{;

typedef MessageServer::MessageDispatcher MessageDispatcher;
class request_handler
{
    virtual void on_request(Context& context) = 0;
};

class server
{
public:

public:
    inline server()
        : message_server_(),
        tcpserver_notify_(message_server_)
    {}

    inline ~server()
    {
        message_server_.stop();
    }

    // 启动服务
    void run(const size_t thread_num);

    // 等待线程完成
    void join();

    // 停止服务
    void stop();

    // 注册消息处理器
    void init(MessageDispatcher& dispatcher);

////////////////////////////////////////////////////////////////////////////////
private:
    // 消息处理
    MessageServer message_server_;

    //以下任选其一
    //tcp消息通知
    TcpserverNotify tcpserver_notify_;
};


////////////////////////////////////////////////////////////////////////////////
}// ns::serverframe
#endif
