#ifndef __REQUEST_ADAPTER_H__
#define __REQUEST_ADAPTER_H__
#include <memory>
namespace serverframe{;

////////////////////////////////////////////////////////////////////////////////
class request_adapter
{
public:
    template<typename message_t, typename handler_t>
    inline static void handle(std::shared_ptr<message_t>& cont)
    {
        std::shared_ptr<handler_t> handler(new handler_t);
        handler->on_request(*cont);
    }
};

////////////////////////////////////////////////////////////////////////////////
}// serverframe
#endif
