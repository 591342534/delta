#ifndef __TCP_ENGINE_H__
#define __TCP_ENGINE_H__
#include "asio/asio_library.h"
#include "singleton.h"
#include <iostream>

namespace serverframe
{

class tcp_engine : public singleton<tcp_engine>
{
private:
    tcp_engine();
    virtual ~tcp_engine();

    friend class singleton<tcp_engine>;

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

}// serverframe
#endif
