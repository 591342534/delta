/*****************************************************************************
 returns_report_manager Copyright (c) 2016. All Rights Reserved.

 FileName: processor_base.h
 Version: 1.0
 Date: 2016.03.15

 History:
 ericsheng     2016.03.15   1.0     Create
 ******************************************************************************/

#ifndef __PROCESSOR_BASE_H__
#define __PROCESSOR_BASE_H__

#include "common.h"
#include "base/mqueue.h"
#include "base/thread.h"

namespace serverframe
{

struct atp_message
{
    int type;
    void* param1;
    void* param2;
};


class message_dispatcher
{
public:
    virtual ~message_dispatcher();
public:
    virtual int dispatch_message(atp_message& msg) = 0;
};

class processor_base
    : public base::process_thread
{
public:
    processor_base()
    {
        msg_queue_ = new base::srt_queue<atp_message>(12);
        msg_queue_->init();
    }

    virtual ~processor_base()
    {
        if(msg_queue_ != NULL) {
            delete msg_queue_;
            msg_queue_ = NULL;
        }
    }

    virtual void post(const atp_message& msg)
    {
        msg_queue_->push(msg);
    }

protected:
    virtual int get(atp_message& msg)
    {
        if(msg_queue_->pop(msg)) {
            return 0;
        }
        else {
            return -1;
        }
    }

protected:
    base::srt_queue<atp_message>* msg_queue_;
};

}

#endif
