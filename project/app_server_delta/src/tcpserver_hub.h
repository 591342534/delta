#ifndef __TCPSERVER_HUB_H__
#define __TCPSERVER_HUB_H__


////////////////////////////////////////////////////////////////////////////////
#include "dispatch_server.h"

namespace serverframe{;
////////////////////////////////////////////////////////////////////////////////
class tcpserver_hub
{
public:

    typedef std::string tcp_context;
    typedef dispatch_server<tcp_context> dispatch_server_alias;
    //server need
    typedef dispatch_server_alias::message_dispatcher message_dispatcher;
    typedef dispatch_server_alias::register_func register_func;

////////////////////////////////////////////////////////////////////////////////
public:
    /*@constructor. */
    tcpserver_hub()
    {
    }

    /*@init handler map which contain the relationship of request and handler.
    */
    inline void register_handle(register_func func)
    {
        m_dispatch_server.register_handle(func);
    }

    /*@start message server and dedicated service size. */
    inline void run(const size_t server_size)
    {
        m_dispatch_server.run(server_size);
    }

    /*@wait message server end. */
    inline void join()
    {
        m_dispatch_server.join();
    }

    /*@stop message server. */
    inline void stop()
    {
        m_dispatch_server.stop();
    }

////////////////////////////////////////////////////////////////////////////////
public:
    /*@ receive data from exist connection. */
    inline void on_read(std::string& data)
    {
        try {
            // package message context.
            std::shared_ptr<tcp_context> cont(new tcp_context(data));
            m_dispatch_server.post(cont);
        }
        catch (std::exception& ec) {
            std::cout << ec.what();
        }
    }

////////////////////////////////////////////////////////////////////////////////
protected:
    // request queue server.
    dispatch_server_alias m_dispatch_server;
};

}//serverframe
////////////////////////////////////////////////////////////////////////////////
#endif