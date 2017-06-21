#ifndef __MESSAGE_SERVER_H__
#define __MESSAGE_SERVER_H__

////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <memory>

#include "nocopyable.h"
#include "state.h"
#include "message_queue.h"
#include "thread_manager.h"
#include "message_dispatcher.h"
namespace serverframe{;
////////////////////////////////////////////////////////////////////////////////
template<typename message_t>
class dispatch_handler
{
public:
    typedef message_dispatcher<message_t> message_dispatcher_alias;

    inline dispatch_handler() {}

    inline void operator()(std::shared_ptr<message_t>& msg)
    {
        m_dispatcher.dispatch(msg);
    }

    message_dispatcher_alias m_dispatcher;
};


////////////////////////////////////////////////////////////////////////////////
template<typename message_t>
class message_server : public nocopyable
{
public:
    typedef message_queue< std::shared_ptr<message_t> > message_queue_alias;
////////////////////////////////////////////////////////////////////////////////
public:
    inline message_server(){}

    inline ~message_server()
    {
        stop();
    }

    /*@ start message server: start dispatch message and process it.
    * @ para.thread_size: how many threads to start, if equal to "0", message 
    server will run in a synchronous mode, else it will start number of threads 
    (decicated by "thread_size")and message server run in a asynchronous mode.
    */
    void run(const size_t thread_size)
    {
        // update server state.
        m_state.run();

        // start work thread, if "thread_size"==0, this server will run in a 
        // synchronous mode and message queue will not be init.
        if (thread_size != 0) {    // asynchronous mode    
            m_message_queue.reset(new message_queue_alias());

            m_thread_manager.reset(new thread_manager);
            m_thread_manager->run_thread(std::bind(&message_server::work, this), thread_size);
        }
        // else {}                // synchronous mode
    }

    /*@ stop message server: stop message queue and thread pool. */
    void stop()
    {
        // update server state to notify work thread.
        m_state.stop();

        // close message queue.
        if (m_message_queue != nullptr) {
            m_message_queue->close();
        }

        // wait server stop.
        join();
    }

    /*@ wait server stop.*/
    inline void join()
    {
        std::shared_ptr<thread_manager> thread_p = m_thread_manager;
        if (thread_p != nullptr) {
            thread_p->join_all();
        }
    }

public:
    /*@ push message to message server.
    * @ if server running in synchronous mode, process this message.
    * @ else push this message in message queue.
    */
    inline void post(std::shared_ptr<message_t>& msg)
    {
        if (m_state.is_stoped()) {
            throw std::logic_error("message server is stopped.");
        }

        std::shared_ptr<message_queue_alias> msg_q = m_message_queue;
        if (msg_q == nullptr) {    // synchronous mode
            handle(msg);
        }
        else {                    // asynchronous mode
            msg_q->post(msg);
        }
    }

    /*@ get the message handler.*/
    inline dispatch_handler<message_t>& get_handler() {
        return m_message_handler;
    }

////////////////////////////////////////////////////////////////////////////////
protected:
    /*@ work thread method.    */
    virtual void work()
    {
        while (m_state.is_running()) {
            std::shared_ptr<message_t> msg;
            if (m_message_queue->take(msg)) {
                handle(msg);
            }
        }// end while
    }

    inline void handle(std::shared_ptr<message_t>& msg)
    {
        try {
            m_message_handler(msg);
        }
        catch (std::exception& err) {
            std::cout << "message dispatch handle error: " << err.what();
        }
        catch (...) {
            std::cout << "message dispatch handle unknown error.";
        }
    }

////////////////////////////////////////////////////////////////////////////////
protected:
    // thread pool.
    std::shared_ptr<thread_manager> m_thread_manager;

    // message queue.
    std::shared_ptr<message_queue_alias> m_message_queue;

    dispatch_handler<message_t> m_message_handler;

    // message server state.
    state m_state;
};


////////////////////////////////////////////////////////////////////////////////
}// serverframe
#endif
