/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: trade_server.h
 Version: 1.0
 Date: 2016.03.21

 History:
 eric     2016.03.21   1.0     Create
 ******************************************************************************/

#ifndef __TRADE_SERVER_H__
#define __TRADE_SERVER_H__

#include "database/dbscope.h"
#include "trade_struct.h"
#include "query_processor.h"
#include "trade_processor.h"
#include "trade_unit.h"
#include "comm_holiday.h"

#include <vector>
#include <string>
#include <thread>
#include <memory>
namespace ctp
{
    struct database_config
    {
        std::string host;
        int port;
        std::string user;
        std::string password;
        std::string dbname;
        database_config()
            : port(3306)
        {}
    };

    struct trade_account_info
    {
        std::string broker;
        std::string userid;
        std::string account;
        std::string password;
    };

    struct trade_server_config
    {
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
        trade_server_config m_server_config;
        std::vector<trade_account_info> ar_accounts_info;

        trade_server_param()
            : server_name("")
            , query_thread_count(1)
            , trade_thread_count(1)
            , unit_thread_count(1)
            , blog_root_path("./")
            , check_interval_(120)
            , m_account_tbl_name_("")
            , m_statutory_holiday_tbl_name_("")
            , m_deal_tbl_name_("")
            , m_entrust_tbl_name_("")
        {
        }
    };



typedef VBASE_HASH_MAP<const char*, trade_unit*, string_hash, string_compare> map_str_trade_unit;
typedef VBASE_HASH_MAP<const char*, trade_account_info, string_hash, string_compare> map_str_account_info;

class trade_server : 
    public message_dispatcher
{
public:
	trade_server();
	virtual ~trade_server();

public:
	int start(const char* config_file);
	int stop();

protected:
    int start_internal();
    int stop_internal();

public:
	virtual int dispatch_message(atp_message& msg);

public:
	trade_server_param& server_param() {
		return params_;
	}

	database::db_conn_pool& get_conn_pool() {
        return m_db_pool_;
	}

	comm_holiday& get_comm_holiday() {
        return m_comm_holiday_;
	}

    map_str_trade_unit& map_tunits() {
        return map_tunits_;
    }

    bool get_server_start_flag(){
        return started_;
    }

   static std::string get_account_broker_bs_key(std::string broker, 
       std::string account, long bs = -1)
   {
       char buf[1024];
       if (bs == -1) {
           sprintf(buf, "%s_%s", broker.c_str(), account.c_str());
       }
       else {
           sprintf(buf, "%s_%s_%ld", broker.c_str(), account.c_str(), bs);
       }
       return buf;
   }

protected:
	int load_config(const char* config_file);
    int init_db_pool();
    int request_account_config();
    int init_localno();

protected:
	void post_rsp_message(atp_message& msg);
	void release_rsp_messages();
	void process_rsp();
	static void process_rsp_thread(void* param);

private:
	int get_index(const char* key, int bound)
    {
        return base::util::hash_key(key) % bound;
    }

private:
	trade_server_param params_;
    database::db_conn_pool m_db_pool_;
    comm_holiday m_comm_holiday_;
    map_str_account_info map_accounts_info_;
	map_str_trade_unit map_tunits_;
	std::vector<trade_processor*> ar_trade_processors_;
	std::vector<query_processor*> ar_query_processors_;

	int localno_;
    bool stop_rsp_thread_;
	std::shared_ptr<std::thread> m_sptr_rsp_thread;
	base::srt_queue<atp_message>* rsp_queue_;
	base::event* rsp_event_;
	bool started_;
};

}

#endif  //__NAUT_CTPTRADE_TRADE_SERVER_H__
