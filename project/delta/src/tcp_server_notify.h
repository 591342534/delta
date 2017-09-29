
#ifndef TCP_NOTIFY_H__
#define TCP_NOTIFY_H__
#include <string>
#include "dispatch_server.h"
#include "utility/session.h"
namespace serverframe{;

typedef dispatch_server<Context> DispatchServer;

//消息回报
class TcpserverNotify : public utility::tcp_notify
{
public:
    TcpserverNotify(DispatchServer& hdl);

protected:
    virtual size_t on_recv_data(const unsigned int clientid, const string& buf);

private:
    DispatchServer& dispatch_server_;

};

}// ns::serverframe
#endif
