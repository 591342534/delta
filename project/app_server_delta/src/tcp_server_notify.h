/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: tcp_server_notify.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#ifndef __TCP_NOTIFY_H__
#define __TCP_NOTIFY_H__
#include <string>
#include "protocal.h"
#include "asio/asio_library.h"
#include "tcpserver_hub.h"

namespace serverframe{
    ;

    //////////////////////////////////////////////////////////////////////////////
    //消息回报
    class tcpserver_message_notify : public asio::asio_message_notify
    {
    public:
        tcpserver_message_notify(tcpserver_hub& hdl);

        ////////////////////////////////////////////////////////
    protected:
        virtual void on_message_stream(const std::string& stream);

    private:
        tcpserver_hub& m_hub;

    };

    ////////////////////////////////////////////////////////////////////////////////
}// ns::serverframe
#endif
