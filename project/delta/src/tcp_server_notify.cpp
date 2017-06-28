/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: tcp_server_notify.cpp
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#include "tcp_server_notify.h"
#include "server.h"
#include "parse.h"
#include <iostream>
using namespace std;
namespace serverframe
{
    //##############################################################################
    //##############################################################################
    //////////////////////////////////////////////////////////

    tcpserver_message_notify::tcpserver_message_notify(tcpserver_hub& handler)
        : m_hub(handler)
    {
    }

    size_t tcpserver_message_notify::on_recv_data(const unsigned int clientid, const string& buf)
    {
        try {
            std::string buff;
            encode(clientid, buf.c_str(), buf.size(), buff);
            m_hub.on_read(const_cast<std::string&>(buff));

            return buf.size();
        }
        catch (std::exception& err) {
            std::cout << "tcpserver_message_notify error: " << err.what();
        }
        catch (...) {
            std::cout << "tcpserver_message_notify unknown error.";
        }
    }
    ////////////////////////////////////////////////////////////////////////////////
}// ns::serverframe
