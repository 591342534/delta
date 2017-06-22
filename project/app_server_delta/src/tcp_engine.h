/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: tcp_engine.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#ifndef __TCP_ENGINE_H__
#define __TCP_ENGINE_H__
#include "asio/asio_library.h"
#include "utility/singleton.h"
#include <iostream>

namespace serverframe
{

    class tcp_engine
    {
        SINGLETON_ONCE_UNINIT(tcp_engine);
        tcp_engine();
    public:
        virtual ~tcp_engine();

    public:
        // 初始化消息队列
        void init(const char *ip, int port, asio::asio_message_notify& ptrNotify);
        void stop();

        asio::asio_server_api* get_tcpserver();
    private:
        asio::asio_server_api* m_tcpserver;
        // 客户端
        //base::TTcpClient*    m_tcpclient;
    };
    SINGLETON_GET(tcp_engine);
}// serverframe
#endif
