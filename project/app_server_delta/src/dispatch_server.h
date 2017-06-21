#ifndef __DISPATCH_SERVER_H__
#define __DISPATCH_SERVER_H__

////////////////////////////////////////////////////////////////////////////////
#include <functional>

#include "message_server.h"
namespace serverframe{;

////////////////////////////////////////////////////////////////////////////////
template<typename message_t>
class dispatch_server :
    public message_server< message_t >
{
public:
    typedef message_server< message_t > super;
    typedef typename dispatch_handler< message_t >::message_dispatcher_alias message_dispatcher;
    typedef std::function<void(message_dispatcher&)> register_func;

    inline dispatch_server() {}

    // init message handler map.
    inline void register_handle(register_func func)
    {
        if (func != nullptr) {
            func(get_dispatcher());
        }
    }

    inline message_dispatcher& get_dispatcher()
    {
        return super::m_message_handler.m_dispatcher;
    }
    inline const message_dispatcher& get_dispatcher() const
    {
        return super::m_message_handler.m_dispatcher;
    }
};
////////////////////////////////////////////////////////////////////////////////
}// serverframe
#endif
