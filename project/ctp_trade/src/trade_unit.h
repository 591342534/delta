/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: trade_unit.h
 Version: 1.0
 Date: 2016.03.21

 History:
 david wang     2016.03.21   1.0     Create
 ******************************************************************************/

#ifndef __NAUT_CTPTRADE_UNIT_H__
#define __NAUT_CTPTRADE_UNIT_H__

#include "ThostFtdcTraderApi.h"
#include "ThostFtdcUserApiStruct.h"
#include "ThostFtdcUserApiDataType.h"
#include "base/thread.h"
#include "base/reference_base.h"
#include "base/event.h"
#include "database/unidb.h"
#include "processor_base.h"
#include <string>
#include <vector>
#include <map>

namespace ctp
{

class trade_server;

enum conn_state {
    cs_init,
    cs_connecting,
    cs_connected,
    cs_disconnecting,
    cs_disconnected
};

struct trade_unit_params
{
	std::string trade_host;
	int trade_port;
	std::string userid;
	std::string account;
	std::string password;
	std::string trade_log_file;
	std::string broker;
	std::string autotrade_entrust_tbl_name;
	std::string autotrade_withdraw_tbl_name;
	bool use_simulation_flag;

	trade_unit_params()
		: trade_host("222.73.106.130")
		, trade_port(7787)
		, userid("demo000801")
		, password("888888")
		, trade_log_file("./")
		, broker("00000000")
	    , use_simulation_flag(false)
	{
	}
};

enum process_requestid_function {
	PROCESS_ENTRUST_RESULT_REQUESTID,
	PROCESS_ENTRUST_DEAL_RESULT_REQUESTID,
	PROCESS_WITHDRAW_RESULT_REQUESTID,
	PROCESS_QRYACCOUNT_RESULT_REQUESTID,
	PROCESS_QRYDEAL_RESULT_REQUESTID,
	PROCESS_QRYOPENINER_RESULT_REQUESTID,
	PROCESS_QRYENTRUST_RESULT_REQUESTID,
};

typedef std::map<int, process_requestid_function> map_int_requestid_function;

class trade_unit
	: public processor_base
{
public:
	trade_unit();
	virtual ~trade_unit();

public:
	int start(const trade_unit_params& params, message_dispatcher* mdpt, trade_server* tserver);

public:
	virtual void post(atp_message& msg);
	virtual void stop();

protected:
	int start_internal();
	int stop_internal();

	virtual void run();
	void release_messages();


private:
	trade_unit_params params_;
	message_dispatcher* mdpt_;
	trade_server* tserver_;

	std::string userid_;
	std::string account_;
	std::string trade_pwd_;
	std::string broker_;

	CThostFtdcTraderApi* trader_;
	bool connected_;
	bool logined_;

	base::event* msg_event_;
	base::event* state_event_;

	bool started_;
	long m_RequestId_;
	int m_FrontID_;
	int m_SessionID_;
	std::string m_MaxOrderRef_;
	map_int_requestid_function map_int_req_fun_;
    int m_stockhold_old_buy_;
    int m_stockhold_old_sell_;
    bool m_force_orders_started_;
    long m_last_respond;
    enum conn_state  m_conn_state;
    base::int64 m_last_qryacc;
    bool m_last_qryacc_finish;

};

}

#endif  //__NAUT_CTPTRADE_UNIT_H__
