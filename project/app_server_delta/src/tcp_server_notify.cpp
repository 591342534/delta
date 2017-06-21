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

void tcpserver_message_notify::on_message_stream(const std::string& stream)
{
    try {
        m_hub.on_read(const_cast<std::string&>(stream));
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
