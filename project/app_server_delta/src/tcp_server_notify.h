
#ifndef __TCP_NOTIFY_H__
#define __TCP_NOTIFY_H__
#include <string>
#include "protocal.h"
#include "asio/asio_library.h"
#include "tcpserver_hub.h"

namespace serverframe{;

#define CHECK_IF_NEW_FLAG(m_new_flag) \
if (m_new_flag) { \
    delete[] tmp_buf; \
    tmp_buf = NULL; \
    m_new_flag = false; \
} \

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
