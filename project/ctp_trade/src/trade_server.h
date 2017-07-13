/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: trade_server.h
 Version: 1.0
 Date: 2016.03.21

 History:
 david wang     2016.03.21   1.0     Create
 ******************************************************************************/

#ifndef __NAUT_CTPTRADE_TRADE_SERVER_H__
#define __NAUT_CTPTRADE_TRADE_SERVER_H__

#include "base/base.h"
#include "base/sigslot.h"
#include "trade_struct.h"
#include "query_processor.h"
#include "trade_processor.h"
#include "special_query_processor.h"
#include "order_checker.h"
#include "trade_unit.h"
#include "comm_holiday.h"
#include "database/unidb.h"
#include "database/unidbpool.h"
#include "base/timer.h"
#include "base/alarm.h"
#include <vector>
#include <string>

namespace ctp
{

typedef VBASE_HASH_MAP<const char*, trade_unit*, string_hash, string_compare> map_str_trade_unit;

typedef VBASE_HASH_MAP<string, vector<string> > map_str_acc;

struct database_config
{
	std::string host;
	int port;

	std::string dbname;
	std::string user;
	std::string password;

	database_config()
		: port(3306)
	{}
};

struct trade_server_config
{
	std::string host;
	int port;

	database_config database;
};

struct trade_account_info
{
	std::string broker;
	std::string userid;
	std::string account;
	std::string password;
};

struct trade_mq_config
{
	std::string name;
	std::string host;
	int port;
};

struct trade_server_param
{
	std::string server_name;

	int query_thread_count;
	int trade_thread_count;
	int unit_thread_count;

	std::string blog_root_path;
	std::string switch_time;
    long check_interval_;

    std::string m_account_tbl_name_;
    std::string m_statutory_holiday_tbl_name_;
    std::string m_deal_tbl_name_;
    std::string m_entrust_tbl_name_;

    database_config server_config_database;
	trade_server_config server_config;

	std::vector<trade_account_info> ar_accounts_info;
	std::vector<std::string> holidaysvec_;  //节假日列表
	trade_server_param()
        : server_name("")
        , query_thread_count(1)
		, trade_thread_count(1)
	    , unit_thread_count(1)
		, blog_root_path("./")
		, switch_time("03:00:00")
	    , check_interval_(120)
        , m_account_tbl_name_("")
        , m_statutory_holiday_tbl_name_("")
        , m_deal_tbl_name_("")
        , m_entrust_tbl_name_("")
	{
	    holidaysvec_.clear();
	}
};

struct mq_progress_info
{
	std::string topic;
	std::string subtopic;
	long recv_index;
	int sub_id;

	mq_progress_info()
		: recv_index(-1)
		, sub_id(-1)
	{}
};

class trade_server
	: public message_dispatcher
	, public sigslot::has_slots<>
{
public:
	trade_server();
	virtual ~trade_server();

public:
	int start(const char* config_file);
	int stop();

public:
	virtual int dispatch_message(atp_message& msg);
//	void alarm_callback(base::alarm_info& ainfo, struct tm* t);

public:
	trade_server_param& server_param() {
		return params_;
	}

	map_str_trade_unit& map_tunits() {
		return map_tunits_;
	}

	database::db_conn_pool* get_conn_pool() {
		return pub_trade_db_pool_;
	}

    database::db_conn_pool* get_risk_conn_pool() {
        return risk_trade_db_pool_;
    }

	bool get_server_start_flag(){
	    return started_;
	}
	comm_holiday* get_comm_holiday() {
	    return p_comm_holiday_;
	}

   static std::string get_account_broker_bs_key(std::string broker, std::string account, long bs = -1);

protected:
	int start_internal();
	int stop_internal();

protected:
	int load_config(const char* config_file, trade_server_param& params);
	int request_server_config(trade_server_param& params);
	int request_account_channel_config(trade_server_param& params);
	int init_localno();
	int init_db_pool();
	int load_statutory_holidays(trade_server_param& params);

protected:
	void post_rsp_message(atp_message& msg);
	void release_rsp_messages();
	void process_rsp();
	static void process_rsp_thread(void* param);

	std::string curr_trade_date();

protected:
	std::string get_subs_subtopic(const char* broker, const char* account);
	std::string get_subs_key(const char* broker, const char* account, const char* subtopic);
	std::string get_subs_key(const char* topic, const char* subtopic);
	int get_index(const char* key, int bound);

private:
	trade_server_param params_;
    database::unidb* trade_db_;
    database::db_conn_pool* pub_trade_db_pool_;
    database::db_conn_pool* risk_trade_db_pool_;

	map_str_trade_unit map_tunits_;
	std::vector<trade_processor*> ar_trade_processors_;
	std::vector<query_processor*> ar_query_processors_;
	std::vector<trade_unit*> ar_trade_units_;
	special_query_processor* squery_processor_;
	order_checker* order_checker_;

	base::timer mq_reconnect_timer_;

	VBASE_HASH_MAP<const char*, trade_account_info, string_hash, string_compare> map_accounts_info_;

	int localno_;

    comm_holiday* p_comm_holiday_;

	base::thread* rsp_thread_;
	bool stop_rsp_thread_;
	base::srt_queue<atp_message>* rsp_queue_;
	base::event* rsp_event_;
	bool started_;
};

}

#endif  //__NAUT_CTPTRADE_TRADE_SERVER_H__