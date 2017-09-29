#include <iostream>
#include "tcp_server_notify.h"
#include "parse.h"

using namespace std;
namespace serverframe
{
    TcpserverNotify::TcpserverNotify(DispatchServer& hdl)
        : dispatch_server_(hdl)
    {
    }

    size_t TcpserverNotify::on_recv_data(const unsigned int clientid, const string& buf)
    {
        try {
            std::string buff;
            encode(clientid, buf.c_str(), buf.size(), buff);
            dispatch_server_.on_read(buff);

            return buf.size();
        }
        catch (std::exception& err) {
            std::cout << "TcpserverNotify error: " << err.what();
        }
        catch (...) {
            std::cout << "TcpserverNotify unknown error.";
        }
    }
}// ns::serverframe
