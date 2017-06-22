/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: tcp_engine.cpp
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/
#include "tcp_engine.h"

namespace serverframe
{
    ////////////////////////////////////////////////////////////////////////////////

    tcp_engine::tcp_engine()
    {
        m_tcpserver = NULL;
    }


    ////////////////////////////////////////////////////////////////////////////////
    tcp_engine::~tcp_engine()
    {
        stop();
    }


    ////////////////////////////////////////////////////////////////////////////////
    void tcp_engine::init(const char *ip, int port,
        asio::asio_message_notify& ptrNotify)
    {
        // 服务端   
        m_tcpserver = asio::create_asio_server(port, &ptrNotify);
        m_tcpserver->start_up();

        std::cout << "tcp_engine starting" << std::endl;
    }


    ////////////////////////////////////////////////////////////////////////////////
    void tcp_engine::stop()
    {
        if (m_tcpserver != NULL)  {
            asio::delete_asio_server(m_tcpserver);
        }
    }

    asio::asio_server_api* tcp_engine::get_tcpserver()
    {
        if (m_tcpserver == NULL) {
            std::cout << "m_tcpserver is null ptr." << std::endl;
        }
        return m_tcpserver;
    }

    ////////////////////////////////////////////////////////////////////////////////
}// serverframe
