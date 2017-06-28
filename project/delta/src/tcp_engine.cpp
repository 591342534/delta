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
        utility::tcp_notify& notify)
    {
        // 服务端   
        m_tcpserver = new utility::tcpserver("127.0.0.1", std::to_string(port), notify);
        utility::get_thread_manager().create_thread(*m_tcpserver);

        std::cout << "tcp_engine starting" << std::endl;
    }


    ////////////////////////////////////////////////////////////////////////////////
    void tcp_engine::stop()
    {
        if (m_tcpserver != nullptr) {
            delete m_tcpserver;
            m_tcpserver = nullptr;
        }
    }

    utility::tcpserver* tcp_engine::get_tcpserver()
    {
        if (m_tcpserver == NULL) {
            std::cout << "m_tcpserver is null ptr." << std::endl;
        }
        return m_tcpserver;
    }

    ////////////////////////////////////////////////////////////////////////////////
}// serverframe
