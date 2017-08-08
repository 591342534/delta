/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: query_processor.h
 Version: 1.0
 Date: 2017.08.08

 History:
 eric     2017.08.08   1.0     Create
 ******************************************************************************/

#ifndef __NAUT_CTPTRADE_QUERY_PROCESSOR_H__
#define __NAUT_CTPTRADE_QUERY_PROCESSOR_H__

#include "trade_struct.h"
#include "processor_base.h"
#include "database/unidbpool.h"
#include "base/event.h"

namespace ctp
{

class trade_server;

class query_processor
	: public processor_base
{
public:
	query_processor();
	virtual ~query_processor();

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
	int process_qry_account(base::dictionary& dict);
	int process_qry_openinter(base::dictionary& dict);
	int process_qry_entrust(base::dictionary& dict);
	int process_qry_checker_entrust(base::dictionary& dict);
	int process_qry_deal(base::dictionary& dict);
	int process_qry_checker_deal(base::dictionary& dict);

	int process_rsp_account(base::dictionary& dict);
	int process_rsp_openinter(base::dictionary& dict);
	int process_rsp_entrust(base::dictionary& dict);
	int process_rsp_checker_entrust(base::dictionary& dict);
	int process_rsp_deal(base::dictionary& dict);
	int process_rsp_checker_deal(base::dictionary& dict);

	int process_cmd_error(base::dictionary& dict);

	void release_messages();
	void check_query_wait_timeout();

private:
	std::string server_name_;
	message_dispatcher* mdpt_;
	trade_server* tserver_;

	base::event* msg_event_;
	std::vector<atp_message> wait_match_queue_;

	VBASE_HASH_MAP<std::string, base::dictionary*> map_dict_account_;
	VBASE_HASH_MAP<std::string, base::dictionary*> map_dict_entrust_;
	VBASE_HASH_MAP<std::string, base::dictionary*> map_dict_openinter_;
	VBASE_HASH_MAP<std::string, base::dictionary*> map_dict_deal_;

	bool started_;
};

}

#endif  //__NAUT_CTPTRADE_QUERY_PROCESSOR_H__