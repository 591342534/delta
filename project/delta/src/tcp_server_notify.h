
#ifndef TCP_NOTIFY_H__
#define TCP_NOTIFY_H__
#include <string>
#include "utility/session.h"
#include "message_server.h"

namespace serverframe{;

typedef message_server<Context> MessageServer;

//消息回报
class TcpserverNotify : public utility::tcp_notify
{
public:
    TcpserverNotify(MessageServer& hdl);

protected:
    virtual size_t on_recv_data(const unsigned int clientid, const string& buf);

private:
    MessageServer& message_server_;

};

}// ns::serverframe
#endif
