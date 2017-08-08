/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: processor_base.h
 Version: 1.0
 Date: 2016.03.21

 History:
 david wang     2016.03.21   1.0     Create
 ******************************************************************************/

#ifndef __NAUT_CTPTRADE_PROCESSOR_BASE_H__
#define __NAUT_CTPTRADE_PROCESSOR_BASE_H__

#include "trade_struct.h"
#include "base/mqueue.h"
#include "base/thread.h"
#include <stdio.h>

namespace ctp
{

class processor_base
	: public base::process_thread
{
public:
	processor_base() {
		msg_queue_ = new base::srt_queue<atp_message>(12);
		msg_queue_->init();
	}
	virtual ~processor_base() {
		if (msg_queue_ != NULL) {
			delete msg_queue_;
			msg_queue_ = NULL;
		}
	}

	/**
	 * post a message to process queue
	 * it's multithread safe, and can be called by multiple threads concurrently
	 * @param msg
	 */
	virtual void post(const atp_message& msg) {
		msg_queue_->push(msg);
	}

protected:
	/**
	 * get a message from process queue
	 * though it's not multithread safe, we call it only in process thread itself to ensure safety.
	 * @param msg[out]
	 */
	virtual int get(atp_message& msg) {
		if (msg_queue_->pop(msg)) {
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

#endif /* __NAUT_CTPTRADE_PROCESSOR_BASE_H__ */
