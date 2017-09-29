/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: dispatch_server.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#ifndef __DISPATCH_SERVER_H__
#define __DISPATCH_SERVER_H__

#include <functional>

#include "message_server.h"
namespace serverframe{;

typedef std::string Context;
typedef std::shared_ptr<Context> ContextPtr;

template<typename message_t>
class dispatch_server : public message_server<message_t>
{
public:
    typedef typename dispatch_handler<message_t>::MessageDispatcher MessageDispatcher;
    typedef std::function<void(MessageDispatcher&)> register_func;

    inline dispatch_server() {}

    // init message handler map.
    inline void register_handle(register_func func)
    {
        if (func != nullptr) {
            func(m_message_handler.m_dispatcher);
        }
    }

    inline void on_read(message_t& data)
    {
        try {
            // package message context.
            std::shared_ptr<message_t> data(new message_t(data));
            post(data);
        }
        catch (std::exception& ec) {
            std::cout << ec.what();
        }
    }

};
}// serverframe
#endif
