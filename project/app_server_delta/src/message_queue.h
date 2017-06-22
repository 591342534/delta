/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: message_queue.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/
#ifndef __MESSAGE_QUEUE_H__
#define __MESSAGE_QUEUE_H__


////////////////////////////////////////////////////////////////////////////////
#include <queue>

#include "utility/nocopyable.h"
#include "utility/state.h"
#include <mutex>
#include <condition_variable>
namespace serverframe
{
    ////////////////////////////////////////////////////////////////////////////////
    template <typename message_t>
    class message_queue : public utility::nocopyable
    {
        ////////////////////////////////////////////////////////////////////////////////
    public:
        // max size.
        enum { kmax_size = 4096 };

        /*@ the message queue max message size.*/
        message_queue(const size_t full_size = kmax_size)
        {
            open(full_size);
        }

        /*@ open message queue so that it can recv message.*/
        void open(const size_t full_size = kmax_size)
        {
            m_full_size = (full_size > 0) ? full_size : kmax_size;
            m_state.ok();
        }

        inline bool is_open() const
        {
            return m_state.isok();
        }

        /*@ close message queue so that it stop to recv message.*/
        void close()
        {
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_state.none();
                while (!m_queue.empty()) {
                    m_queue.pop();
                }
            }

            // notify all thread to exit message queue.
            m_empty_cond_var.notify_all();
            m_full_cond_var.notify_all();
        }

        inline bool is_close() const
        {
            return m_state.isnone();
        }

        /*@ post message to message queue.*/
        void post(message_t& msg)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (is_full() && !is_close()) {
                m_full_cond_var.wait(lock);
            }
            if (!is_full() && !is_close()) {
                m_queue.push(msg);
                m_empty_cond_var.notify_one();
            }
        }

        /*@ pop message from this message queue.*/
        bool take(message_t& msg)
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (is_empty() && !is_close()) {
                m_empty_cond_var.wait(lock);
            }
            if (!is_empty() && !is_close()) {
                msg = m_queue.front();
                m_queue.pop();
                m_full_cond_var.notify_one();
                return true;
            }
            return false;
        }

    private:
        inline bool is_empty() const
        {
            return m_queue.empty();
        }

        inline bool is_full() const
        {
            return (m_queue.size() >= m_full_size);
        }

        ////////////////////////////////////////////////////////////////////////////////
    private:
        //  message queue max size.
        size_t m_full_size;

        // message queue data.
        std::queue<message_t> m_queue;

        // when message queue is full and empty, synchronous notify.
        std::mutex m_mutex;
        std::condition_variable m_full_cond_var;
        std::condition_variable m_empty_cond_var;

        utility::state m_state;
    };

}// serverframe
#endif
