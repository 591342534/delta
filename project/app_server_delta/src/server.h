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
#include "tcp_engine.h"
#include "tcp_server_notify.h"
#include "tcpserver_hub.h"
#include "protocal.h"
namespace serverframe{;

typedef tcpserver_hub::tcp_context context;
typedef std::shared_ptr<context> context_ptr;
typedef tcpserver_hub::message_dispatcher message_dispather;

class request_handler
{
    virtual void on_request(context& context) = 0;
};

class server
{
public:
    inline server()
        : m_hub(), 
        m_tcp_notify(m_hub)
    {}

    inline ~server()
    {
        m_hub.stop();
    }

    // 启动服务
    void run(const size_t thread_num);

    // 等待线程完成
    void join();

    // 注册消息处理器
    void init(tcpserver_hub::message_dispatcher& dispatcher);

    // 停止服务
    void stop();

    inline tcpserver_hub& get_hub()
    {
        return m_hub;
    }

////////////////////////////////////////////////////////////////////////////////
private:
    // 消息处理
    tcpserver_hub m_hub;

    //以下任选其一
    //tcp消息通知
    tcpserver_message_notify m_tcp_notify;
};


////////////////////////////////////////////////////////////////////////////////
}// ns::serverframe
#endif
