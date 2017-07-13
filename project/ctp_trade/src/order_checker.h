/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: deal_checker.h
 Version: 1.0
 Date: 2016.03.21

 History:
 david wang     2016.03.21   1.0     Create
 ******************************************************************************/

#ifndef __NAUT_CTPTRADE_DEAL_CHECKER_H__
#define __NAUT_CTPTRADE_DEAL_CHECKER_H__

#include "base/thread.h"
#include "database/unidbpool.h"
#include "trade_struct.h"
#include "processor_base.h"
#include "base/event.h"

namespace ctp
{
class trade_server;

class order_checker
	: public processor_base
{
public:
	order_checker();
	virtual ~order_checker();

public:
	int start(const char* server_name, message_dispatcher* mdpt, trade_server* ts);

public:
	virtual void post(const atp_message& msg);
	virtual void stop();

protected:
	int start_internal();
	int stop_internal();

protected:
	virtual void start();
	virtual void run();

protected:
	std::string get_check_id();

	int check_deal();
	int process_rsp_checker_deal(base::dictionary& dict);
	int process_rsp_checker_single_deal(base::dictionary& dict);

	int check_entrust();
	int process_rsp_checker_entrust(base::dictionary& dict);
	int process_rsp_checker_single_entrust(base::dictionary& dict);

	void release_messages();

protected:
	std::string replace_quote(std::string& text);

private:
	std::string server_name_;
	message_dispatcher* mdpt_;
	trade_server* tserver_;
	base::event* msg_event_;
	long last_check_time_;
	long check_interval_;
	int check_flag_;
	int check_id_;
	long wait_timeout_;

	VBASE_HASH_MAP<std::string, long> map_entrust_timeout_;
	VBASE_HASH_MAP<std::string, long> map_deal_timeout_;

	bool started_;
};

}

#endif  //__NAUT_CTPTRADE_DEAL_CHECKER_H__