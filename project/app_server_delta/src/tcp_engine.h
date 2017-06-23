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
#include "utility/singleton.h"
#include "utility/tcpserver.h"
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
        void init(const char* ip, int port, utility::tcp_notify& notify);
        void stop();

        utility::tcpserver* get_tcpserver();
    private:
        utility::tcpserver* m_tcpserver;
        // 客户端
        //base::TTcpClient*    m_tcpclient;
    };
    SINGLETON_GET(tcp_engine);
}// serverframe
#endif
