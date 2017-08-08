/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: trade_processor.h
 Version: 1.0
 Date: 2017.08.08

 History:
 eric     2017.08.08   1.0     Create
 ******************************************************************************/

#ifndef _TRADE_PROCESSOR_H__
#define _TRADE_PROCESSOR_H__

#include "trade_struct.h"
#include "processor_base.h"
#include "base/mqueue.h"
#include "base/event.h"
#include "database/unidbpool.h"

namespace ctp
{

class trade_server;

class trade_processor
	: public processor_base
{
public:
	trade_processor();
	virtual ~trade_processor();

public:
	int start(const char* server_name, message_dispatcher* mdpt, trade_server* ts);
    virtual void stop();

public:
	virtual void post(const atp_message& msg);

protected:
	int start_internal();
	int stop_internal();
    void release_messages();

protected:
	virtual void run();

protected:
	void process_withdraws();
    int process_withdraw(base::dictionary& dict);
	int process_entrust(base::dictionary& dict);
	int process_entrust_response(base::dictionary& dict);
	int process_order_status_response(base::dictionary& dict);
	int process_deal(base::dictionary& dict);

	/* withdraw response is not supported by current zd server */
	int process_withdraw_response(base::dictionary& dict);
	int process_systemno_response(base::dictionary& dict);
	int process_cmd_error(base::dictionary& dict);

protected:
	std::string replace_quote(std::string& text);

private:
	std::string server_name_;
	message_dispatcher* mdpt_;
	trade_server* tserver_;

	base::event* msg_event_;
	base::srt_queue<atp_message>* withdraw_queue_;
	std::vector<atp_message> withdraw_wait_queue_;

	bool started_;
};

}

#endif  //__NAUT_CTPTRADE_ENTRUST_PROCESSOR_H__