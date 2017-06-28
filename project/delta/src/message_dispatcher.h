/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: message_dispatcher.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#ifndef __MESSAGE_DISPATCHER_H__
#define __MESSAGE_DISPATCHER_H__
#include <unordered_map>
#include <string.h>
#include <functional>
#include "request_adapter.h"
#include "parse.h"
namespace serverframe{;

template<typename message_t>
class dispatcher
{
public:
    typedef std::function<void (std::shared_ptr<message_t>&)> handler_func;

protected:
    // message and message handler map.
    typedef std::unordered_map<int, handler_func> handler_map;
    typedef typename handler_map::const_iterator const_iterator;

public:
    dispatcher()
    {}

    /*@ dispatch message to message handler.
    * @ para.msg: message to be dispatched.
    */
    inline void dispatch(std::shared_ptr<message_t>& msg) const
    {
        // get message type id.
        int type = get_message_type(*msg);
        const_iterator it = m_handler_map.find(type);
        if (it != m_handler_map.end()) {
            it->second(msg);
        }
        else {
            m_default_handler(msg);
        }
    }

public:
    handler_map m_handler_map;

    // default message handler to process unregistered message type.
    handler_func m_default_handler;
};



////////////////////////////////////////////////////////////////////////////////
template<typename message_t>
class message_dispatcher : public dispatcher<message_t>
{
public:
    typedef dispatcher<message_t> super;
    typedef typename super::handler_func handler_func;

    /*@ register message and message handler, and message dedicated by typeid.
    * @ tmpl.message: message type to be registered.
    * @ tmpl.handler_t: to handle "tmpl.message" type.
    * @ para.id: message type id to identify message type.
    */
    template<typename handler_t>
    inline void route(int id)
    {
        handler_func func = &request_adapter::handle<message_t, handler_t>;
        super::m_handler_map[id] = func;
    }

    /*@ register default message handler to process unregistered message type.
    * @ tmpl.handler_t: default message handler.
    */
    template<typename handler_t>
    inline void route_default()
    {
        handler_func func = &request_adapter::handle<message_t, handler_t>;
        super::m_default_handler = func;
    }
};

}// serverframe
#endif
