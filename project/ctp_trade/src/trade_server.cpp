/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: trade_server.cpp
 Version: 1.0
 Date: 2016.03.21

 History:
 david wang     2016.03.21   1.0     Create
 ******************************************************************************/

#include "base/aes.h"
#include "trade_server.h"
#include "common.h"
#include "version.h"
#include "base/pugixml.hpp"
#include "base/trace.h"
#include "base/base.h"
#include "base/util.h"
#include "base/dictionary.h"
#include "base/reference_base.h"
#include "database/unidb.h"
#include "base/dtrans.h"
#include "progress_recorder.h"

namespace ctp
{

trade_server::trade_server()
	: trade_db_(NULL)
	, pub_trade_db_pool_(NULL)
    , risk_trade_db_pool_(NULL)
	, squery_processor_(NULL)
	, order_checker_(NULL)
	, mqc_(NULL)
	, localno_(time(NULL))
	, rsp_thread_(NULL)
	, stop_rsp_thread_(false)
	, rsp_queue_(NULL)
	, rsp_event_(NULL)
	, started_(false)
//    , m_alarm_(NULL)
    , p_comm_holiday_(NULL)
{
    m_at_accountchannel_tbl_name_ = "at_accountchannel";
    m_autotrade_deal_tbl_name_ = "autotrade_deal";
    m_autotrade_entrust_tbl_name_ = "autotrade_entrust";
    m_autotrade_withdraw_tbl_name_ = "autotrade_withdraw";
    m_statutory_holiday_tbl_name_ = "statutory_holidays";
}

trade_server::~trade_server()
{
	stop();
}

int trade_server::start(const char* config_file)
{
	TRACE_SYSTEM(AT_TRACE_TAG, "------------------- start trader server -------------------");

	if (started_) {
		return NAUT_AT_S_OK;
	}

	int ret = load_config(config_file, params_);
	CHECK_LABEL(ret, end);

	ret = request_server_config(params_);
	CHECK_LABEL(ret, end);

    ret = load_statutory_holidays(params_);
    CHECK_LABEL(ret, end);

	if(params_.use_simulation_flag) {
        ret = request_account_channel_config(params_);
        CHECK_LABEL(ret, end);
	} else {
        ret = load_statutory_holidays(params_);
        CHECK_LABEL(ret, end);
        p_comm_holiday_ = new comm_holiday(this);
    }
	ret = start_internal();
	CHECK_LABEL(ret, end);

end:
	if (BSUCCEEDED(ret)) {
		started_ = true;
	}
	else {
		stop();
	}
	return ret;
}

int trade_server::stop()
{
	TRACE_SYSTEM(AT_TRACE_TAG, "------------------- stop trader server -------------------");

	started_ = false;
	return stop_internal();
}

int trade_server::dispatch_message(atp_message& msg)
{
	if (!started_) {
		ref_dictionary* rd = (ref_dictionary*)msg.param1;
		rd->release();
		return NAUT_AT_S_OK;
	}

	switch (msg.type) {
	case ATP_MESSAGE_TYPE_IN_ORDER:
		{
			ref_dictionary* rd = (ref_dictionary*)msg.param1;
			assert(rd != NULL);

			if(ar_trade_processors_.size() > 0) {
                int index = get_index((*(rd->get()))["account"].string_value().c_str(),
                        (int)ar_trade_processors_.size());
                TRACE_SYSTEM(AT_TRACE_TAG, "ar_trade_processors_ post msg:[ %s ] ~~~", rd->get()->to_string().c_str());
                ar_trade_processors_[index]->post(msg);
			} else {
			    TRACE_WARNING(AT_TRACE_TAG, "ar_trade_processors_ size is 0 ~~~~");
			}
		}
		break;
	case ATP_MESSAGE_TYPE_SERVER_TRADE_RSP:
		{
			ref_dictionary* rd = (ref_dictionary*)msg.param1;
			assert(rd != NULL);
			if(ar_trade_processors_.size() > 0) {
                int index = get_index((*(rd->get()))["account"].string_value().c_str(),
                        (int)ar_trade_processors_.size());
                ar_trade_processors_[index]->post(msg);
			} else {
			    TRACE_WARNING(AT_TRACE_TAG, "ar_trade_processors_ size is 0 ~~~~");
			}
		}
		break;
	case ATP_MESSAGE_TYPE_IN_QUERY:
	case ATP_MESSAGE_TYPE_CHECKER_REQ:
	case ATP_MESSAGE_TYPE_SERVER_QUERY_RSP:
		{
			ref_dictionary* rd = (ref_dictionary*)msg.param1;
			assert(rd != NULL);

			if(ar_query_processors_.size() > 0) {
                int index = get_index((*(rd->get()))["account"].string_value().c_str(),
                        (int)ar_query_processors_.size());
                ar_query_processors_[index]->post(msg);
			} else {
			    TRACE_WARNING(AT_TRACE_TAG, "ar_query_processors_ size is 0 ~~~~");
			}
		}
		break;
	case ATP_MESSAGE_TYPE_DEAL_PUSH:
	case ATP_MESSAGE_TYPE_QUERY_REPLY:
	case ATP_MESSAGE_TYPE_ORDER_REPLY:
		post_rsp_message(msg);
		break;
	case ATP_MESSAGE_TYPE_SERVER_TRADE_REQ:
	case ATP_MESSAGE_TYPE_SERVER_QUERY_REQ:
		{
			ref_dictionary* rd = (ref_dictionary*)msg.param1;
			assert(rd != NULL);

			std::string account = (*(rd->get()))["account"].string_value();
			std::string broker = (*(rd->get()))["broker"].string_value();
			std::string account_key = get_account_broker_bs_key(broker, account);
			if (map_tunits_.find(account_key.c_str()) != map_tunits_.end()) {
				map_tunits_[account_key.c_str()]->post(msg);
			}
			else {
				TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_MISMATCHED,
						"the trade unit with specified account '%s' is not found,",
						account_key.c_str());
			}
		}
		break;
	case ATP_MESSAGE_TYPE_CHECKER_RSP:
		if(order_checker_ != NULL) {
		    order_checker_->post(msg);
		}
		break;
	case ATP_MESSAGE_TYPE_IN_SPECIAL_QUERY:
		if(squery_processor_ != NULL) {
		    squery_processor_->post(msg);
		}
		break;
	default:
		TRACE_WARNING(AT_TRACE_TAG, "unknown message type: %d", msg.type);
		break;
	}
	return NAUT_AT_S_OK;
}

int trade_server::start_internal()
{
	int ret = NAUT_AT_S_OK;

	LABEL_SCOPE_START;

	/* initialize database */
	naut::unidb_param dbparam;
	dbparam.create_database_if_not_exists = false;
	dbparam.recreate_database_if_exists = false;
	dbparam.host = params_.server_config_database.host;
	dbparam.port = params_.server_config_database.port;
	dbparam.user = params_.server_config_database.user;
	dbparam.password = params_.server_config_database.password;
	dbparam.database_name = params_.server_config_database.dbname;

	trade_db_ = new naut::unidb();
	ret = trade_db_->open(dbparam);
	CHECK_LABEL(ret, end);

    ret = init_db_pool();
    CHECK_LABEL(ret, end);

	/* initialize localno */
	ret = init_localno();
	CHECK_LABEL(ret, end);

	/* initialize account channel */
    for (int i = 0; i < (int)params_.account_channel_info.size(); i++)
    {
        account_channel& acc_channel = params_.account_channel_info[i];
        std::string channel_key = get_account_broker_bs_key(acc_channel.broker, acc_channel.account, acc_channel.bs_type);
        map_account_channel::iterator iter = map_account_channel_bs_.find(channel_key);
        if(iter != map_account_channel_bs_.end()) {
            iter->second.chanelvec.push_back(acc_channel.channel);
        } else {
            account_channel_vec acc_chanel_vec;
            acc_chanel_vec.chanelvec.push_back(acc_channel.channel);
            map_account_channel_bs_[channel_key] = acc_chanel_vec;
        }
    }

	/* initialize trade units */
	for (int i = 0; i < (int)params_.ar_accounts_info.size(); i++)
	{
		trade_unit_params tparams;
		tparams.trade_host = params_.server_config.host;
		tparams.trade_port = params_.server_config.port;
		tparams.userid = params_.ar_accounts_info[i].userid;
		tparams.password = params_.ar_accounts_info[i].password;
		tparams.account = params_.ar_accounts_info[i].account;
		tparams.broker = params_.ar_accounts_info[i].broker;
		tparams.autotrade_entrust_tbl_name = m_autotrade_entrust_tbl_name_;
		tparams.autotrade_withdraw_tbl_name = m_autotrade_withdraw_tbl_name_;
		tparams.use_simulation_flag = params_.use_simulation_flag;
		tparams.trade_log_file = params_.blog_root_path + params_.ar_accounts_info[i].account + "/";

		std::string account_key_str = get_account_broker_bs_key(tparams.broker, tparams.account);

		if (map_tunits_.find(account_key_str.c_str()) != map_tunits_.end()) {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNTS_DUPLICATED,
					"duplicate account found, %s", account_key_str.c_str());
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNTS_DUPLICATED, end);
		}
        trade_unit* tunit = new trade_unit();
        ret = tunit->start(tparams, this, this);
        if (BFAILED(ret)) {
          delete tunit;
          CHECK_LABEL(ret, end);
        }
        char* account_key = new char[account_key_str.length() + 1];
        strcpy(account_key, account_key_str.c_str());

        map_tunits_[account_key] = tunit;
        map_accounts_info_[account_key] = params_.ar_accounts_info[i];
	}

	/* start processors */
	for (int i = 0; i < params_.trade_thread_count; i++) {
		trade_processor* ep = new trade_processor();
		ar_trade_processors_.push_back(ep);
		ret = ep->start(params_.server_name.c_str(), this, this);
		CHECK_LABEL(ret, end);
	}

	for (int i = 0; i < params_.query_thread_count; i++) {
		query_processor* qp = new query_processor();
		ar_query_processors_.push_back(qp);
		ret = qp->start(params_.server_name.c_str(), this, this);
		CHECK_LABEL(ret, end);
	}

	if(params_.use_simulation_flag) {
        squery_processor_ = new special_query_processor();
        ret = squery_processor_->start(this, this);
        CHECK_LABEL(ret, end);
	}
	order_checker_ = new order_checker();
	ret = order_checker_->start(params_.server_name.c_str(), this,this);
	CHECK_LABEL(ret, end);

	/* initialize progress recorder */
	char file_path[256];
	sprintf(file_path, "%s%s/progress_%s.log",
			params_.blog_root_path.c_str(), "progress", curr_trade_date().c_str());
	ret = progress_recorder::shared_instance().init(file_path);
	CHECK_LABEL(ret, end);

	/* initialize mq response references */
	rsp_queue_ = new base::srt_queue<atp_message>(10);
	rsp_queue_->init();

	rsp_event_ = new base::event();

	/* connect to mq server */
	mqc_ = nio::mqclient::create_client();
	if (!mqc_->init(params_.mq_config.host.c_str(), params_.mq_config.port)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONNECT_MQ_FAILED,
				"connect to mq failed, host: %s, port: %d",
				params_.mq_config.host.c_str(), params_.mq_config.port);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_MQ_FAILED, end);
	}
	mqc_->signal_state_.connect(this, &trade_server::on_state_changed);
	mqc_->signal_message_.connect(this, &trade_server::on_incoming_message);

	/* subscribe to mq */
	for (int i = 0; i < (int)params_.ar_accounts_info.size(); i++)
	{
		std::string subtopic = get_subs_subtopic(params_.ar_accounts_info[i].broker.c_str(),
				params_.ar_accounts_info[i].account.c_str());

		char key[512];
		sprintf(key, "%s_%s_%s", MQ_TOPIC_TRADE, params_.ar_accounts_info[i].broker.c_str(),
				params_.ar_accounts_info[i].account.c_str());
		mq_progress_info* pi = new mq_progress_info();
		pi->topic = MQ_TOPIC_TRADE;
		pi->subtopic = subtopic;
		pi->sub_id = -1;
		map_subs_by_topic_[key] = pi;
		progress_info* pgi = progress_recorder::shared_instance().get_progress_info(subtopic.c_str());
		if (pgi != NULL){
			pi->recv_index = pgi->processed_index;
		}
		else {
			pi->recv_index = -1;
		}

		sprintf(key, "%s_%s_%s", MQ_TOPIC_QUERY, params_.ar_accounts_info[i].broker.c_str(),
				params_.ar_accounts_info[i].account.c_str());
		pi = new mq_progress_info();
		pi->topic = MQ_TOPIC_QUERY;
		pi->subtopic = subtopic;
		pi->sub_id = -1;
		pi->recv_index = -1;
		map_subs_by_topic_[key] = pi;
	}
	do_subscriptions();

	/* start response process thread */
	stop_rsp_thread_ = false;
	rsp_thread_ = new base::thread();
	rsp_thread_->create(process_rsp_thread, this);

//    base::alarm_info alarminfo;
//    sscanf(params_.switch_time.c_str(), "%d:%d:%d", &alarminfo.hour, &alarminfo.minute, &alarminfo.second);
//    m_alarm_ = new base::alarm(alarminfo.hour, alarminfo.minute, alarminfo.second);
//    m_alarm_->signal_alarm_.connect(this, &trade_server::alarm_callback);
//    m_alarm_->turn_on();
	LABEL_SCOPE_END;

end:
	return ret;
}


int trade_server::init_db_pool()
{
    int ret = NAUT_AT_S_OK;

    if(pub_trade_db_pool_ != NULL) {
        pub_trade_db_pool_->release_all_conns();
        delete pub_trade_db_pool_;
        pub_trade_db_pool_ = NULL;
    }

    if(risk_trade_db_pool_ != NULL) {
        risk_trade_db_pool_->release_all_conns();
        delete risk_trade_db_pool_;
        risk_trade_db_pool_ = NULL;
    }

    LABEL_SCOPE_START;

    // pub_trade_db_pool_ initialize database
    naut::unidb_param dbparam_pub;
    dbparam_pub.create_database_if_not_exists = false;
    dbparam_pub.recreate_database_if_exists = false;
    dbparam_pub.host = params_.server_config.database.host;
    dbparam_pub.port = params_.server_config.database.port;
    dbparam_pub.user = params_.server_config.database.user;
    dbparam_pub.password = params_.server_config.database.password;
    dbparam_pub.database_name = params_.server_config.database.dbname;

    std::string str_db_type = "mysql";
    int conn_count = 1;
    pub_trade_db_pool_ = new naut::db_conn_pool();
    ret = pub_trade_db_pool_->init(dbparam_pub, conn_count, str_db_type, params_.switch_time);
    CHECK_LABEL(ret, end);

    if (params_.use_simulation_flag) {
        // risk_trade_db_pool_ initialize database
        naut::unidb_param dbparam_risk;
        dbparam_risk.create_database_if_not_exists = false;
        dbparam_risk.recreate_database_if_exists = false;
        dbparam_risk.host = params_.server_config_risk_database.host;
        dbparam_risk.port = params_.server_config_risk_database.port;
        dbparam_risk.user = params_.server_config_risk_database.user;
        dbparam_risk.password = params_.server_config_risk_database.password;
        dbparam_risk.database_name = params_.server_config_risk_database.dbname;

        std::string str_db_type2 = "mysql";
        int conn_count2 = 1;
        risk_trade_db_pool_ = new naut::db_conn_pool();
        ret = risk_trade_db_pool_->init(dbparam_risk, conn_count2, str_db_type2, params_.switch_time);
        CHECK_LABEL(ret, end);
    }
    LABEL_SCOPE_END;

end:
    return ret;
}

int trade_server::stop_internal()
{
	/* reset mq reconnecte timer */
	mq_reconnect_timer_.reset();

	/* stop mq response thread */
	if (rsp_thread_ != NULL) {
		stop_rsp_thread_ = true;
		rsp_thread_->join();
		delete rsp_thread_;
		rsp_thread_ = NULL;
	}

	/* stop mq */
	if (mqc_ != NULL) {
		mqc_->shutdown();
		nio::mqclient::destroy_client(mqc_);
		mqc_ = NULL;
	}

	/* stop processors */
	for (int i = 0; i < (int)ar_trade_processors_.size(); i++) {
		if (ar_trade_processors_[i] != NULL) {
			ar_trade_processors_[i]->stop();
			delete ar_trade_processors_[i];
		}
	}
	ar_trade_processors_.clear();

	for (int i = 0; i < (int)ar_query_processors_.size(); i++) {
		if (ar_query_processors_[i] != NULL) {
			ar_query_processors_[i]->stop();
			delete ar_query_processors_[i];
		}
	}
	ar_query_processors_.clear();

   for (int i = 0; i < (int)ar_trade_units_.size(); i++) {
        if (ar_trade_units_[i] != NULL) {
            ar_trade_units_[i]->stop();
            delete ar_trade_units_[i];
        }
    }
    ar_trade_units_.clear();

	if (order_checker_ != NULL) {
		order_checker_->stop();
		delete order_checker_;
		order_checker_ = NULL;
	}

	if (squery_processor_ != NULL) {
		squery_processor_->stop();
		delete squery_processor_;
		squery_processor_ = NULL;
	}

	/* stop trade unit */
	map_accounts_info_.clear();
	VBASE_HASH_MAP<const char*, trade_unit*, string_hash, string_compare>::iterator mit = map_tunits_.begin();
	for (; mit != map_tunits_.end(); mit++) {
		const char* account = mit->first;
		trade_unit* tunit = mit->second;
		if (tunit != NULL) {
			tunit->stop();
			delete tunit;
			tunit = NULL;
		}
		delete[]account;

	}
	map_tunits_.clear();

	/* release progress info */
	VBASE_HASH_MAP<std::string, mq_progress_info*>::iterator sit = map_subs_by_topic_.begin();
	for (; sit != map_subs_by_topic_.end(); ) {
		if (sit->second) {
			delete sit->second;
			sit++;
		}
	}
	map_subs_by_topic_.clear();
	map_subs_by_id_.clear();

	/* release response reference resource */
	if (rsp_queue_ != NULL) {
		release_rsp_messages();
		delete rsp_queue_;
		rsp_queue_ = NULL;
	}
	if (rsp_event_ != NULL) {
		delete rsp_event_;
		rsp_event_ = NULL;
	}

	/* close database */
	if (trade_db_ != NULL) {
		trade_db_->close();
		delete trade_db_;
		trade_db_ = NULL;
	}

	map_account_channel_bs_.clear();

	/* release shared progress recorder */
	progress_recorder::release_shared_instance();

	/* reset trade server param */
	params_ = trade_server_param();

//    if(m_alarm_ != NULL) {
//        m_alarm_->turn_off();
//        delete m_alarm_;
//        m_alarm_ = NULL;
//    }

	if(pub_trade_db_pool_ != NULL) {
		delete pub_trade_db_pool_;
		pub_trade_db_pool_ = NULL;
	}

    if(risk_trade_db_pool_ != NULL) {
        delete risk_trade_db_pool_;
        risk_trade_db_pool_ = NULL;
    }

	return NAUT_AT_S_OK;
}


int trade_server::load_statutory_holidays(trade_server_param& params)
{
    int ret = NAUT_AT_S_OK;
     naut::unidb* db = NULL;
     naut::unidb_conn* dbconn = NULL;

     LABEL_SCOPE_START;

     naut::unidb_param dbparam;
     dbparam.host = params.server_config_database.host;
     dbparam.port = params.server_config_database.port;
     dbparam.database_name = params.server_config_database.dbname;
     dbparam.user = params.server_config_database.user;
     dbparam.password = params.server_config_database.password;
     dbparam.create_database_if_not_exists = false;
     dbparam.recreate_database_if_exists = false;

     db = new naut::unidb("mysql");
     int dbret = db->open(dbparam);
     if (BFAILED(dbret)) {
         TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED,
                 "open config database failed, ret: %d, host: %s, port: %d, dbname: %s, user: %s",
                 dbret, dbparam.host.c_str(), dbparam.port, dbparam.database_name.c_str(), dbparam.user.c_str());
         ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED, end);
     }

     dbconn = new naut::unidb_conn(db);
     if (!dbconn->init_conn()) {
         TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_INIT_DATABASE_CONN_FAILED,
                 "init dbconn of the config database failed");
         ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED, end);
     }

     /* query at_accountchannel */
     char sql[512];

     sprintf(sql,"select * from %s", m_statutory_holiday_tbl_name_.c_str());

     if(dbconn->query(sql))
     {
         params.holidaysvec_.clear();
         while(dbconn->fetch_row())
         {
             std::string holiday = dbconn->get_string("holiday");

             if(holiday.empty())
             {
                 continue;
             }

             params.holidaysvec_.push_back(holiday);
         }
     }

     LABEL_SCOPE_END;

 end:
     if (BFAILED(ret)) {
         params.holidaysvec_.clear();
     }
     if (dbconn != NULL) {
         dbconn->release_conn();
         delete dbconn;
     }
     if (db != NULL) {
         db->close();
         delete db;
     }
     return ret;
}

int trade_server::load_config(const char* config_file, trade_server_param& params)
{
	assert(config_file != NULL);

	int ret = NAUT_AT_S_OK;

	LABEL_SCOPE_START;

	pugi::xml_document doc;
	if (!doc.load_file(config_file)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIGFILE_INVALID,
				"ctp-trade: config file is not exist or invalid '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIGFILE_INVALID, end);
	}

	pugi::xml_node xroot = doc.child("ctp-trade");
	if (xroot.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp-trade: root element 'ctp-trade' should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}

	/* server name */
	std::string tmp = xroot.child("server-name").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp-trade: server name should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
	params.server_name = tmp;

    pugi::xml_node xmoa_reg_con = xroot.child("moa-register");
    tmp = xmoa_reg_con.text().as_string();
    if (tmp == "true") {
    	params.moa_register = true;
     } else {
    	params.moa_register = false;
     }

	/* server config database */
	pugi::xml_node xserver_config_database = xroot.child("server-config-database");
	if (xserver_config_database.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp-trade: server config database should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}

	tmp = xserver_config_database.child("host").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp-trade: host of the server config database should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
	params.server_config_database.host = tmp;

	tmp = xserver_config_database.child("port").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp-trade: port of the server config database should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
	params.server_config_database.port = atoi(tmp.c_str());

	tmp = xserver_config_database.child("dbname").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp-trade: dbname of the server config database should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
	params.server_config_database.dbname = tmp;

	tmp = xserver_config_database.child("user").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp-trade: user of the server config database should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
	params.server_config_database.user = tmp;

	tmp = xserver_config_database.child("password").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp-trade: password of the server config database should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}

	//20151116 david wang
	base::aes rsapwd;
	std::string eplain = rsapwd.decrypt_base64(tmp.c_str(), tmp.length());

	params.server_config_database.password = eplain;

    /* server config risk database */
    pugi::xml_node xserver_simulation_trade = xroot.child("simulation-trade");
    if (!xserver_simulation_trade.empty()) {
        tmp = xserver_simulation_trade.text().as_string();
        if(tmp == "true") {
            params.use_simulation_flag = true;
        }
    }

    if(params.use_simulation_flag) {
        /* server config risk database */
        pugi::xml_node xserver_config_risk_database = xroot.child("server-config-risk-database");
        if (xserver_config_risk_database.empty()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
                    "ctp-trade: server config database should be specified, '%s'", config_file);
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
        }

        tmp = xserver_config_risk_database.child("host").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
                    "ctp-trade: host of the server config database should be specified, '%s'", config_file);
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
        }
        params.server_config_risk_database.host = tmp;

        tmp = xserver_config_risk_database.child("port").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
                    "ctp-trade: port of the server config database should be specified, '%s'", config_file);
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
        }
        params.server_config_risk_database.port = atoi(tmp.c_str());

        tmp = xserver_config_risk_database.child("dbname").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
                    "ctp-trade: dbname of the server config database should be specified, '%s'", config_file);
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
        }
        params.server_config_risk_database.dbname = tmp;

        tmp = xserver_config_risk_database.child("user").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
                    "ctp-trade: user of the server config database should be specified, '%s'", config_file);
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
        }
        params.server_config_risk_database.user = tmp;

        tmp = xserver_config_risk_database.child("password").text().as_string();
        if (tmp.empty()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
                    "ctp-trade: password of the server config database should be specified, '%s'", config_file);
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
        }

        eplain = rsapwd.decrypt_base64(tmp.c_str(), tmp.length());

        params.server_config_risk_database.password = eplain;
    }

	/* processor config */
	pugi::xml_node xprocessor_config = xroot.child("processor-config");
	if (xprocessor_config.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp-trade: element 'processor-config' should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}

	tmp = xprocessor_config.child("query-thread-count").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp-trade: query-thread-count should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
	params.query_thread_count = atoi(tmp.c_str());

	tmp = xprocessor_config.child("trade-thread-count").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp-trade: trade-thread-count should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
	params.trade_thread_count = atoi(tmp.c_str());

   tmp = xprocessor_config.child("unit-thread-count").text().as_string();
   if (tmp.empty()) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
                "ctp-trade: unit-thread-count should be specified, '%s'", config_file);
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
    }
    params.unit_thread_count = atoi(tmp.c_str());

	/* blog root path */
	tmp = xroot.child("blog-root-path").text().as_string();
	if (!tmp.empty()) {
		params.blog_root_path = tmp;
		if (params.blog_root_path.rfind("/") != params.blog_root_path.length() - 1) {
			params.blog_root_path += "/";
		}
	}

	/* switch time */
	tmp = xroot.child("switch-time").text().as_string();
	if (!tmp.empty()) {
		bool valid = false;
		int hour, minute, second;
		if (sscanf(tmp.c_str(), "%02d:%02d:%02d", &hour, &minute, &second) != 0) {
			if ((hour >= 0 && hour < 24) && (minute >= 0 && minute < 60) && (second >= 0 && second < 60)) {
				valid = true;
			}
		}
		if (valid) {
			params.switch_time = tmp;
		}
		else {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
					"ctp-trade: format of switch time is invalid, '%s'", tmp.c_str());
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
		}
	}

    /* check-interval path */
    int intvalue = xroot.child("check-interval").text().as_int();
    if (intvalue != 0) {
        params.check_interval_ = intvalue;
    }

	/* mq server config */
	pugi::xml_node xmqserver = xroot.child("mqserver");
	if (xmqserver.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp-trade: element mqserver should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}

	params.mq_config.name = xmqserver.child("name").text().as_string();
	params.mq_config.host = xmqserver.child("host").text().as_string();
	if (params.mq_config.host.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp-trade: host of the mqserver should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}

	tmp = xmqserver.child("port").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp-trade: port of the mqserver should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
	params.mq_config.port = atoi(tmp.c_str());

	LABEL_SCOPE_END;

end:
	return ret;
}

int trade_server::request_server_config(trade_server_param& params)
{
	int ret = NAUT_AT_S_OK;

	naut::unidb* db = NULL;
	naut::unidb_conn* dbconn = NULL;

	LABEL_SCOPE_START;

	naut::unidb_param dbparam;
	dbparam.host = params.server_config_database.host;
	dbparam.port = params.server_config_database.port;
	dbparam.database_name = params.server_config_database.dbname;
	dbparam.user = params.server_config_database.user;
	dbparam.password = params.server_config_database.password;
	dbparam.create_database_if_not_exists = false;
	dbparam.recreate_database_if_exists = false;

	db = new naut::unidb("mysql");
	int dbret = db->open(dbparam);
	if (BFAILED(dbret)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED,
				"open config database failed, ret: %d, host: %s, port: %d, dbname: %s, user: %s",
				dbret, dbparam.host.c_str(), dbparam.port, dbparam.database_name.c_str(), dbparam.user.c_str());
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED, end);
	}

	dbconn = new naut::unidb_conn(db);
	if (!dbconn->init_conn()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_INIT_DATABASE_CONN_FAILED,
				"init dbconn of the config database failed");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED, end);
	}

	/* query server config */
	char sql[512];
	sprintf(sql, "select trade_host, trade_db_host, trade_db_name, trade_db_user,"
			"trade_db_password from server_config where server_name='%s'", params.server_name.c_str());
	if (!dbconn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_QUERY_SERVER_CONFIG_FAILED,
				"query server config failed, db error: (%d:%s)", dbconn->get_errno(), dbconn->get_error().c_str());
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_QUERY_SERVER_CONFIG_FAILED, end);
	}

	if (dbconn->get_count() == 0 || !dbconn->fetch_row()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_SERVER_CONFIG_NOT_EXIST,
				"config of the specified server_name is not exist");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_SERVER_CONFIG_NOT_EXIST, end);
	}

	std::string tmp = dbconn->get_string("trade_host");
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_SERVER_CONFIG_INVALID,
				"server-config: trade host should be specified");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_SERVER_CONFIG_INVALID, end);
	}
	if (!base::util::parse_server_addr(tmp, params.server_config.host, params.server_config.port)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_SERVER_CONFIG_INVALID,
				"server-config: trade host seems invalid");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_SERVER_CONFIG_INVALID, end);
	}

	tmp = dbconn->get_string("trade_db_host");
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_SERVER_CONFIG_INVALID,
				"server-config: host of the trade database should be specified");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_SERVER_CONFIG_INVALID, end);
	}
	if (!base::util::parse_server_addr(tmp, params.server_config.database.host,
			params.server_config.database.port)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_SERVER_CONFIG_INVALID,
				"server-config: trade database host seems invalid");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_SERVER_CONFIG_INVALID, end);
	}

	params.server_config.database.dbname = dbconn->get_string("trade_db_name");
	if (params.server_config.database.dbname.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_SERVER_CONFIG_INVALID,
				"server-config: name of the trade database should be specified");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_SERVER_CONFIG_INVALID, end);
	}

	params.server_config.database.user = dbconn->get_string("trade_db_user");
	if (params.server_config.database.user.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_SERVER_CONFIG_INVALID,
				"server-config: user of the trade database should be specified");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_SERVER_CONFIG_INVALID, end);
	}

	//20151116 david wang
	tmp = dbconn->get_string("trade_db_password");
	base::aes rsapwd;
	std::string eplain = rsapwd.decrypt_base64(tmp.c_str(), tmp.length());
	params.server_config.database.password = eplain;

	if (params.server_config.database.password.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_SERVER_CONFIG_INVALID,
				"server-config: password of the trade database should be specified");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_SERVER_CONFIG_INVALID, end);
	}

	/* query trade account config */
	sprintf(sql, "select userid, password, account, broker from account_config "
			"where server_name='%s'", params.server_name.c_str());
	if (!dbconn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_QUERY_ACCOUNT_CONFIG_FAILED,
				"query account config failed, db error: (%d:%s)", dbconn->get_errno(), dbconn->get_error().c_str());
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_QUERY_ACCOUNT_CONFIG_FAILED, end);
	}

	if (dbconn->get_count() == 0) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_CONFIG_NOT_EXIT,
				"account config of the specified server_name is not exist");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNT_CONFIG_NOT_EXIT, end);
	}

	while (dbconn->fetch_row()) {
		trade_account_info account_info;
		account_info.userid = dbconn->get_string("userid");
		if (account_info.userid.empty()) {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_CONFIG_INVALID,
					"account-config: userid of the account should be specified");
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNT_CONFIG_INVALID, end);
		}

		//20151116 david wang
		std::string tmp = dbconn->get_string("password");
		base::aes rsapwd;
		std::string eplain = rsapwd.decrypt_base64(tmp.c_str(), tmp.length());

		account_info.password = eplain;
		if (account_info.password.empty()) {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_CONFIG_INVALID,
					"account-config: password of the account should be specified");
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNT_CONFIG_INVALID, end);
		}

		account_info.account = dbconn->get_string("account");
		if (account_info.account.empty()) {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_CONFIG_INVALID,
					"account-config: capital account of the account should be specified");
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNT_CONFIG_INVALID, end);
		}

		account_info.broker = dbconn->get_string("broker");
		if (account_info.broker.empty()) {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_CONFIG_INVALID,
					"account-config: broker of the account should be specified");
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNT_CONFIG_INVALID, end);
		}
		params.ar_accounts_info.push_back(account_info);
	}

	LABEL_SCOPE_END;

end:
	if (BFAILED(ret)) {
		params.ar_accounts_info.clear();
	}
	if (dbconn != NULL) {
		dbconn->release_conn();
		delete dbconn;
	}
	if (db != NULL) {
		db->close();
		delete db;
	}
	return ret;
}

int trade_server::request_account_channel_config(trade_server_param& params)
{
    int ret = NAUT_AT_S_OK;
    naut::unidb* db = NULL;
    naut::unidb_conn* dbconn = NULL;

    LABEL_SCOPE_START;

    naut::unidb_param dbparam;
    dbparam.host = params.server_config_risk_database.host;
    dbparam.port = params.server_config_risk_database.port;
    dbparam.database_name = params.server_config_risk_database.dbname;
    dbparam.user = params.server_config_risk_database.user;
    dbparam.password = params.server_config_risk_database.password;
    dbparam.create_database_if_not_exists = false;
    dbparam.recreate_database_if_exists = false;

    db = new naut::unidb("mysql");
    int dbret = db->open(dbparam);
    if (BFAILED(dbret)) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED,
                "open config database failed, ret: %d, host: %s, port: %d, dbname: %s, user: %s",
                dbret, dbparam.host.c_str(), dbparam.port, dbparam.database_name.c_str(), dbparam.user.c_str());
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED, end);
    }

    dbconn = new naut::unidb_conn(db);
    if (!dbconn->init_conn()) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_INIT_DATABASE_CONN_FAILED,
                "init dbconn of the config database failed");
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED, end);
    }

    /* query at_accountchannel */
    char sql[512];
    sprintf(sql, "select broker, account, flag, svrname,"
            "bs_type, channel from %s where flag = 1 and svrname = '%s' ", m_at_accountchannel_tbl_name_.c_str(), params.server_name.c_str());
    if (!dbconn->query(sql)) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_QUERY_SERVER_CONFIG_FAILED,
                "query at_accountchannel failed, db error: (%d:%s)", dbconn->get_errno(), dbconn->get_error().c_str());
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_QUERY_SERVER_CONFIG_FAILED, end);
    }

    if (dbconn->get_count() == 0) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_SERVER_CONFIG_NOT_EXIST,
                "config of the specified at_accountchannel is not exist");
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_SERVER_CONFIG_NOT_EXIST, end);
    }
    while (dbconn->fetch_row()) {
        account_channel account_info;
        account_info.account = dbconn->get_string("account");
        if (account_info.account.empty()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_CONFIG_INVALID,
                    "at_accountchannel: capital account of the account should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNT_CONFIG_INVALID, end);
        }

        account_info.broker = dbconn->get_string("broker");
        if (account_info.broker.empty()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_CONFIG_INVALID,
                    "at_accountchannel: broker of the account should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNT_CONFIG_INVALID, end);
        }

        account_info.svrname = dbconn->get_string("svrname");
        if (account_info.svrname.empty()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_CONFIG_INVALID,
                    "at_accountchannel: broker of the account should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNT_CONFIG_INVALID, end);
        }

        account_info.channel = dbconn->get_string("channel");
        if (account_info.channel.empty()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_CONFIG_INVALID,
                    "at_accountchannel: broker of the account should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNT_CONFIG_INVALID, end);
        }

        account_info.bs_type = dbconn->get_long("bs_type");
        account_info.flag = dbconn->get_long("flag");

        params.account_channel_info.push_back(account_info);
    }

    LABEL_SCOPE_END;

end:
    if (BFAILED(ret)) {
        params.account_channel_info.clear();
    }
    if (dbconn != NULL) {
        dbconn->release_conn();
        delete dbconn;
    }
    if (db != NULL) {
        db->close();
        delete db;
    }
    return ret;
}

std::string trade_server::get_account_broker_bs_key(std::string broker, std::string account, long bs)
{
    char buf[1024];
    if(bs == -1) {
        sprintf(buf, "%s_%s", broker.c_str(), account.c_str());
      } else {
        sprintf(buf, "%s_%s_%ld", broker.c_str(), account.c_str(), bs);
      }
    return buf;
}

int trade_server::init_localno()
{
	assert(trade_db_ != NULL);

	int ret = NAUT_AT_S_OK;

	naut::unidb_conn* db_conn = new naut::unidb_conn(trade_db_);

	LABEL_SCOPE_START;

	if (!db_conn->init_conn()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_INIT_DATABASE_CONN_FAILED,
				"init dbconn of the trade database failed");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_INIT_DATABASE_CONN_FAILED, end);
	}

	char sql[2048];
	sprintf(sql, "select max(localno) as max_localno from entrust");
	if (!db_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
				"select max localno from database failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
	}

	if (db_conn->fetch_row()) {
		localno_ = (int)db_conn->get_long("max_localno") + 10000;
	}
	else {
		localno_ = 1;
	}

	LABEL_SCOPE_END;

end:
	db_conn->release_conn();
	delete db_conn;
	return ret;
}

void trade_server::on_state_changed(nio::mqclient* mqc, int sub_id, int state, void* param)
{
	switch (state) {
	case nio::MQCLIENT_STATE_CONNECTED:
		TRACE_SYSTEM(AT_TRACE_TAG, "server mq connected");
		break;
	case nio::MQCLIENT_STATE_SUBSCRIBE_SUCCESS:
		{
			nio::mqsubscribe_info* minfo = (nio::mqsubscribe_info*)param;
			assert(minfo != NULL);
			std::string key = get_subs_key(minfo->topic.c_str(), minfo->sub_cond.c_str());
			VBASE_HASH_MAP<std::string, mq_progress_info*>::iterator mit = map_subs_by_topic_.find(key);

			assert(mit != map_subs_by_topic_.end());
			assert(mit->second != NULL);

			mit->second->sub_id = sub_id;
			map_subs_by_id_[sub_id] = mit->second;

			TRACE_SYSTEM(AT_TRACE_TAG, "subscibe topic: %s, subtopic: %s successfully",
					minfo->topic.c_str(), minfo->sub_cond.c_str());
		}
		break;
	case nio::MQCLIENT_STATE_SUBSCRIBE_FAILED:
		{
			nio::mqsubscribe_info* minfo = (nio::mqsubscribe_info*)param;
			assert(minfo != NULL);
			std::string key = get_subs_key(minfo->topic.c_str(), minfo->sub_cond.c_str());
			VBASE_HASH_MAP<std::string, mq_progress_info*>::iterator mit = map_subs_by_topic_.find(key);

			assert(mit != map_subs_by_topic_.end());
			assert(mit->second != NULL);

			mq_progress_info* pinfo = mit->second;
			assert(pinfo != NULL);

			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_MQ_SUBSCRIBE_FAILED,
					"subscribe to mq failed, topic: %s, subtopic: %s, error msg: %s",
					pinfo->topic.c_str(), pinfo->subtopic.c_str(), minfo->extra_msg.c_str());
		}
		break;
	case nio::MQCLIENT_STATE_UNSUBSCRIBE_SUCCESS:
		break;
	case nio::MQCLIENT_STATE_UNSUBSCRIBE_FAILED:
		break;
	case nio::MQCLIENT_STATE_DISCONNECTED:
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_MQ_DISCONNECTED,
				"mqclient is disconnected");
		break;
	case nio::MQCLIENT_STATE_CLOSED:
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_MQ_DISCONNECTED,
				"mqclient is closed, and will reconnect in 2 seconds");
		on_mq_disconnected();
		break;
	}
}

void trade_server::on_incoming_message(nio::mqclient* mqc, int sub_id, short type,
		long msg_index, const char* reserved, const char* data, int data_size)
{
	assert(data != NULL);

	int ret = NAUT_AT_S_OK;

	char* package = new char[data_size + 1];
	memcpy(package, data, data_size);
	package[data_size] = 0;

	TRACE_SYSTEM(AT_TRACE_TAG, "incoming message: %s", package);

	mq_progress_info* pinfo = map_subs_by_id_[sub_id];
	assert(pinfo != NULL);

	pinfo->recv_index = msg_index;

	base::dictionary* dict = base::djson::str2dict(package);
	if (dict == NULL) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_MQ_MESSAGE_INVALID,
				"receive a message that is not a valid json string, %s", package);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_MQ_MESSAGE_INVALID, end);
	}

	LABEL_SCOPE_START;

	std::string account = (*dict)["account"].string_value();
	if (account.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_MQ_MESSAGE_INVALID,
				"acount field is missing or empty, %s", package);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_MQ_MESSAGE_INVALID, end);
	}
   std::string broker = (*dict)["broker"].string_value();
   if (broker.empty()) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_MQ_MESSAGE_INVALID,
                "broker field is missing or empty, %s", package);
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_MQ_MESSAGE_INVALID, end);
    }
   std::string account_key = get_account_broker_bs_key(broker, account);
	if (map_accounts_info_.find(account_key.c_str()) == map_accounts_info_.end()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_MQ_MESSAGE_INVALID,
				"the account is not exist in local account list, %s", package);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_MQ_MESSAGE_INVALID, end);
	}

	if ((*dict)["userid"].string_value().empty()) {
		(*dict)["userid"] = map_accounts_info_[account_key.c_str()].userid;
	}

    if ((*dict)["excode"].string_value().empty()) {
        (*dict)["excode"] = "SHFE";
    }

	(*dict)["msg_index"] = msg_index;
	if (pinfo->topic == MQ_TOPIC_TRADE)
	{
		progress_recorder::shared_instance().record_progress(
				(*dict)["broker"].string_value().c_str(),
				(*dict)["account"].string_value().c_str(),
				(*dict)["orderid"].string_value().c_str(),
				msg_index, ORDER_RECV);

		if ((*dict)["cmd"].string_value() == ATPM_CMD_ENTRUST) {
			char tmp[11];
			sprintf(tmp, "%09d", localno_++);
			(*dict)["localno"] = tmp;
		}
		ref_dictionary* rd = new ref_dictionary(dict);
		atp_message am;
		am.type = ATP_MESSAGE_TYPE_IN_ORDER;
		am.param1 = (void*)rd;
		dispatch_message(am);
	}
	else if (pinfo->topic == MQ_TOPIC_QUERY)
	{
		ref_dictionary* rd = new ref_dictionary(dict);
		atp_message am;
		if ((*dict)["cmd"].string_value() == ATPM_CMD_QRY_DEAL_ALL) {
			am.type = ATP_MESSAGE_TYPE_IN_SPECIAL_QUERY;
		}
		else {
			am.type = ATP_MESSAGE_TYPE_IN_QUERY;
		}
		am.param1 = (void*)rd;
		dispatch_message(am);
	}
	else {
		assert(false);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_MQ_MESSAGE_INVALID, end);
	}

	LABEL_SCOPE_END;

end:
	if (BFAILED(ret)) {
		if (dict != NULL) {
			delete dict;
		}
	}
	delete[]package;
}

void trade_server::do_subscriptions(mq_progress_info* pinfo)
{
	assert(mqc_ != NULL);

	if (pinfo == NULL) {
		VBASE_HASH_MAP<std::string, mq_progress_info*>::iterator mit = map_subs_by_topic_.begin();
		for (; mit != map_subs_by_topic_.end(); mit++) {
			mq_progress_info* pinfo = mit->second;
			assert(pinfo != NULL);
			if (pinfo->sub_id == -1) {
				mqc_->subscribe_subtopic(pinfo->topic.c_str(),
						pinfo->subtopic.c_str(), "RESERVE", pinfo->recv_index + 1);
			}
		}
	}
	else {
		if (pinfo->sub_id == -1) {
			mqc_->subscribe_subtopic(pinfo->topic.c_str(),
					pinfo->subtopic.c_str(), "RESERVE", pinfo->recv_index + 1);
		}
	}
}

void trade_server::on_mq_disconnected()
{
	if (!started_) {
		return;
	}

	VBASE_HASH_MAP<std::string, mq_progress_info*>::iterator mit = map_subs_by_topic_.begin();
	for (; mit != map_subs_by_topic_.end(); mit++) {
		mq_progress_info* pinfo = mit->second;
		assert(pinfo != NULL);
		pinfo->sub_id = -1;
	}
	mq_reconnect_timer_.set_timer(this, on_mq_reconnect_callback, mqc_, 2000);
	mq_reconnect_timer_.start();
}

void trade_server::on_mq_reconnect()
{
	mq_reconnect_timer_.reset();

	if (started_) {
		mqc_->shutdown();
		if (!mqc_->init(params_.mq_config.host.c_str(), params_.mq_config.port)) {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONNECT_MQ_FAILED,
					"connect to mq failed, host: %s, port: %d",
					params_.mq_config.host.c_str(), params_.mq_config.port);
			/* it will reconnect to mq later in mq state callback */
		}
		else {
			do_subscriptions();
		}
	}
}

void trade_server::on_mq_reconnect_callback(void* obj, void* param)
{
	TRACE_DEBUG(AT_TRACE_TAG, "on mq reconnect callback");

	trade_server* ts = (trade_server*)obj;
	assert(param == ts->mqc_);
	ts->on_mq_reconnect();
}

void trade_server::post_rsp_message(atp_message& msg)
{
	rsp_queue_->push(msg);

	if (started_) {
		assert(rsp_event_ != NULL);
		rsp_event_->set();
	}
}

void trade_server::release_rsp_messages()
{
	assert(rsp_queue_ != NULL);

	atp_message msg;
	while (rsp_queue_->pop(msg)) {
		switch (msg.type) {
		case ATP_MESSAGE_TYPE_DEAL_PUSH:
		case ATP_MESSAGE_TYPE_ORDER_REPLY:
		case ATP_MESSAGE_TYPE_QUERY_REPLY:
			{
				ref_dictionary* rd = (ref_dictionary*)msg.param1;
				if (rd != NULL) {
					rd->release();
				}
			}
			break;
		default:
			break;
		}
	}
}

void trade_server::process_rsp()
{
	atp_message msg;
	while (!stop_rsp_thread_) {
		bool processed = false;
		if (mqc_->connected())
		{
			if (!rsp_queue_->pop(msg)) {
				rsp_event_->reset();
				rsp_event_->wait(20);
				continue;
			}

			ref_dictionary* rd = (ref_dictionary*)msg.param1;
			assert(rd != NULL);

			std::string subtopic = get_subs_subtopic((*rd->get())["broker"].string_value().c_str(), (*rd->get())["account"].string_value().c_str());

			TRACE_SYSTEM(AT_TRACE_TAG, "send response: %s", rd->get()->to_string().c_str());

			assert(mqc_ != NULL);
			switch (msg.type) {
			case ATP_MESSAGE_TYPE_DEAL_PUSH:
			case ATP_MESSAGE_TYPE_ORDER_REPLY:
				{
					std::string result = base::djson::dict2str(rd->get());
					if (!mqc_->publish(1, MQ_TOPIC_RSP_TRADE, subtopic.c_str(), result.c_str(), result.length() + 1)) {
						TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_MQ_PUBLISH_MESSAGE_FAILED,
								"send deal push message failed, will retry later, '%s'", result.c_str());
						rd->retain();
						rsp_queue_->push(msg);
					}
				}
				break;
			case ATP_MESSAGE_TYPE_QUERY_REPLY:
				{
					std::string result = base::djson::dict2str(rd->get());
					TRACE_DEBUG(AT_TRACE_TAG,
							"send query response message dict: '%s'", result.c_str());
					if (!mqc_->publish(1, MQ_TOPIC_RSP_QUERY, subtopic.c_str(), result.c_str(), result.length() + 1)) {
						TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_MQ_PUBLISH_MESSAGE_FAILED,
								"send query response message failed, will retry later, '%s'", result.c_str());
						rd->retain();
						rsp_queue_->push(msg);
					} else {
					    TRACE_DEBUG(AT_TRACE_TAG, "topic:%s, subtopic:%s, dict: [%s]", MQ_TOPIC_RSP_QUERY, subtopic.c_str(), result.c_str());
					}
				}
				break;
			default:
				break;
			}

			processed = true;
			rd->release();
		}

		if (!processed) {
			rsp_event_->reset();
			rsp_event_->wait(20);
		}
	}
}

void trade_server::process_rsp_thread(void* param)
{
	trade_server* ts = (trade_server*)param;
	ts->process_rsp();
}

std::string trade_server::curr_trade_date()
{
	long t = time(NULL);
	int hour, minute, second;
	if (sscanf(params_.switch_time.c_str(), "%02d:%02d:%02d", &hour, &minute, &second) != 0) {
		t -= (hour * 3600 + minute * 60 + second);
	}
	return base::util::date_string(t);
}

std::string trade_server::get_subs_subtopic(const char* broker, const char* account)
{
	char topic[256];
	sprintf(topic, "%s_%s", broker, account);
	return topic;
}

std::string trade_server::get_subs_key(const char* broker, const char* account, const char* subtopic)
{
	char key[256];
	sprintf(key, "%s_%s_%s", broker, account, subtopic);
	return key;
}

std::string trade_server::get_subs_key(const char* topic, const char* subtopic)
{
	char key[256];
	sprintf(key, "%s_%s", topic, subtopic);
	return key;
}

int trade_server::get_index(const char* key, int bound)
{
	return base::util::hash_key(key) % bound;
}

//void trade_server::alarm_callback(base::alarm_info& ainfo, struct tm* t)
//{
//    TRACE_SYSTEM(AT_TRACE_TAG, "on alarm now~~~");
//    if(started_) {
//        started_ = false;
//        int ret = init_db_pool();
//        if(ret != NAUT_AT_S_OK) {
//            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_INIT_DATABASE_CONN_FAILED, "init db pool failed :%d", ret);
//        } else {
//            started_ = true;
//        }
//    }
//}

int get_response_error_code(int ret, const char* server_error)
{
	if (server_error != NULL && strlen(server_error) != 0) {
		return AT_ERROR_TRADER;
	}
	switch (ret) {
	case NAUT_AT_S_OK:
		return 0;
	case NAUT_AT_E_EXCUTE_DB_FAILED:
	case NAUT_AT_E_EXCUTE_DB_QUERY_FAILED:
		return AT_ERROR_DATABASE_FAILED;
	case NAUT_AT_E_ORDER_EXPIRED:
		return AT_ERROR_ORDER_EXPIRED;
	case NAUT_AT_E_ORDER_EXISTED:
		return AT_ERROR_ORDER_EXISTED;
	case NAUT_AT_E_ORDER_NOT_EXIST:
		return AT_ERROR_ORDERID_NOT_EXIST;
	case NAUT_AT_E_SYSTEMNO_NOT_EXIST:
		return AT_ERROR_SYSTEMNO_NOT_EXIST;
	case NAUT_AT_E_ORDER_HAS_BEEN_DEALED:
		return AT_ERROR_ORDER_HAS_BEEN_DEALED;
	case NAUT_AT_E_ORDERID_SHOULD_SPECIFIED:
		return AT_ERROR_ORDERID_SHOULD_SPECIFIED;
	case NAUT_AT_E_ENTRUSTNO_NOT_EXIST:
		return AT_ERROR_ENTRUSTNO_NOT_EXIST;
	case NAUT_AT_E_ORDER_HAS_BEEN_WITHDRAWED:
		return AT_ERROR_ORDER_HAS_BEEN_WITHDRAWED;
	case NAUT_AT_E_ORDER_IS_FAILED:
		return AT_ERROR_ORDER_IS_FAILED;
	case NAUT_AT_E_SQP_CONNECT_FAILED:
		return AT_ERROR_CONNECT_SERVER_FAILED;
	case NAUT_AT_E_SQL_WAIT_RESPONSE_TIMEOUT:
		return AT_ERROR_QUERY_TIMEOUT;
	case NAUT_AT_E_ACCOUNT_NOT_EXIST:
		return AT_ERROR_ACCOUNT_NOT_EXIST;
	case NAUT_AT_E_SEND_TRADEINFO_FAILED:
		return AT_ERROR_SEND_TRADEINFO_FAILED;


	case NAUT_AT_E_CTPTRADE_GET_INSTANCE_FAILED:
	case NAUT_AT_E_CTPTRADE_INIT_FAILED:
	case NAUT_AT_E_CTPTRADE_REGISTER_CALLBACK_FAILED:
	case NAUT_AT_E_CTPTRADE_REGISTER_FRONT_FAILED:
	case NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED:
	case NAUT_AT_E_CTPTRADE_NOT_CONNECTED:
	case NAUT_AT_E_CTPTRADE_ACCOUNT_NOT_MATCHED:
	case NAUT_AT_E_CTPTRADE_LOGIN_FAILED:
	    return AT_ERROR_SEND_TRADE_INFO_FAILED;

	default:
		return AT_ERROR_UNSPECIFIED;
	}
}

std::string get_response_error_msg(int error_code, const char* server_error)
{
	static VBASE_HASH_MAP<std::string, std::string> map_server_error;
	static VBASE_HASH_MAP<int, std::string> map_order_error;
	static bool init = false;
	if (!init) {
	    map_order_error[AT_ERROR_TRADER] = "交易服务返回错误";
        map_order_error[AT_ERROR_ORDERID_NOT_EXIST] = "指定订单不存在（比如查询）";
        map_order_error[AT_ERROR_DEAL_NOT_EXIST] = "指定订单的成交不存在";
        map_order_error[AT_ERROR_ORDER_EXISTED] = "订单已存在（比如重复订单）";
        map_order_error[AT_ERROR_ORDER_EXPIRED] = "订单已失效（超时）";
        map_order_error[AT_ERROR_DATABASE_FAILED] = "数据操作失败";
        map_order_error[AT_ERROR_SYSTEMNO_NOT_EXIST] = "订单系统号不存在";
        map_order_error[AT_ERROR_ORDER_HAS_BEEN_DEALED] = " 订单已成交（比如撤单指令）";
        map_order_error[AT_ERROR_ORDERID_SHOULD_SPECIFIED] = "订单号必须指定";
        map_order_error[AT_ERROR_WITHDRAW_FAILED] = " 撤单失败";
        map_order_error[AT_ERROR_ENTRUSTNO_NOT_EXIST] = "委托号不存在 ";
        map_order_error[AT_ERROR_ORDER_HAS_BEEN_WITHDRAWED] = " 订单已被撤单 ";
        map_order_error[AT_ERROR_ORDER_IS_FAILED] = "订单失败";
        map_order_error[AT_ERROR_CONNECT_SERVER_FAILED] = "连接服务失败";
        map_order_error[AT_ERROR_QUERY_TIMEOUT] = "查询已超时";
        map_order_error[AT_ERROR_ACCOUNT_NOT_EXIST] = "指定资金帐号不存在";
        map_order_error[AT_ERROR_SEND_TRADEINFO_FAILED] = "发送交易信息失败";
        map_order_error[AT_ERROR_ORDER_HAS_BEEN_WITHDRAWED_OR_IS_FAILED] = " 订单已被撤单 or 订单失败 ";
        map_order_error[AT_ERROR_UNSPECIFIED] = " 未指定错误 ";
        map_order_error[AT_ERROR_FUNCTION_NOT_EXIST] = " 该函数功能不支持 ";
        map_order_error[AT_ERROR_WITHDRAW_FROM_EXCODE] = " 可能是委托单被退回做撤单处理比如超过每秒发送委托数量或者委托参数不对 ";
        map_order_error[AT_ERROR_SEND_TRADE_INFO_FAILED] = " 交易服务实例连接服务失败或者发送消息失败 ";

		map_order_error[AT_ERROR_CTP_NONE] = "CTP:正确";
		map_order_error[AT_ERROR_CTP_INVALID_DATA_SYNC_STATUS] = "CTP:不在已同步状态";
		map_order_error[AT_ERROR_CTP_INCONSISTENT_INFORMATION] = "CTP:会话信息不一致";
		map_order_error[AT_ERROR_CTP_INVALID_LOGIN] = "CTP:不合法的登录";
		map_order_error[AT_ERROR_CTP_USER_NOT_ACTIVE] = "CTP:用户不活跃";
		map_order_error[AT_ERROR_CTP_DUPLICATE_LOGIN] = "CTP:重复的登录";
		map_order_error[AT_ERROR_CTP_NOT_LOGIN_YET] = "CTP:还没有登录";
		map_order_error[AT_ERROR_CTP_NOT_INITED] = "CTP:还没有初始化";
		map_order_error[AT_ERROR_CTP_FRONT_NOT_ACTIVE] = "CTP:前置不活跃";
		map_order_error[AT_ERROR_CTP_NO_PRIVILEGE] = "CTP:无此权限";
		map_order_error[AT_ERROR_CTP_CHANGE_OTHER_PASSWORD] = "CTP:修改别人的口令";
		map_order_error[AT_ERROR_CTP_USER_NOT_FOUND] = "CTP:找不到该用户";
		map_order_error[AT_ERROR_CTP_BROKER_NOT_FOUND] = "CTP:找不到该经纪公司";
		map_order_error[AT_ERROR_CTP_INVESTOR_NOT_FOUND] = "CTP:找不到投资者";
		map_order_error[AT_ERROR_CTP_OLD_PASSWORD_MISMATCH] = "CTP:原口令不匹配";
		map_order_error[AT_ERROR_CTP_BAD_FIELD] = "CTP:报单字段有误";
		map_order_error[AT_ERROR_CTP_INSTRUMENT_NOT_FOUND] = "CTP:找不到合约";
		map_order_error[AT_ERROR_CTP_INSTRUMENT_NOT_TRADING] = "CTP:合约不能交易";
		map_order_error[AT_ERROR_CTP_NOT_EXCHANGE_PARTICIPANT] = "CTP:经纪公司不是交易所的会员";
		map_order_error[AT_ERROR_CTP_INVESTOR_NOT_ACTIVE] = "CTP:投资者不活跃";
		map_order_error[AT_ERROR_CTP_NOT_EXCHANGE_CLIENT] = "CTP:投资者未在交易所开户";
		map_order_error[AT_ERROR_CTP_NO_VALID_TRADER_AVAILABLE] = "CTP:该交易席位未连接到交易所";
		map_order_error[AT_ERROR_CTP_DUPLICATE_ORDER_REF] = "CTP:报单错误：不允许重复报单";
		map_order_error[AT_ERROR_CTP_BAD_ORDER_ACTION_FIELD] = "CTP:错误的报单操作字段";
		map_order_error[AT_ERROR_CTP_DUPLICATE_ORDER_ACTION_REF] = "CTP:撤单已报送，不允许重复撤单";
		map_order_error[AT_ERROR_CTP_ORDER_NOT_FOUND] = "CTP:撤单找不到相应报单";
		map_order_error[AT_ERROR_CTP_INSUITABLE_ORDER_STATUS] = "CTP:报单已全成交或已撤销，不能再撤";
		map_order_error[AT_ERROR_CTP_UNSUPPORTED_FUNCTION] = "CTP:不支持的功能";
		map_order_error[AT_ERROR_CTP_NO_TRADING_RIGHT] = "CTP:没有报单交易权限";
		map_order_error[AT_ERROR_CTP_CLOSE_ONLY] = "CTP:只能平仓";
		map_order_error[AT_ERROR_CTP_OVER_CLOSE_POSITION] = "CTP:平仓量超过持仓量";
		map_order_error[AT_ERROR_CTP_INSUFFICIENT_MONEY] = "CTP:资金不足";
		map_order_error[AT_ERROR_CTP_DUPLICATE_PK] = "CTP:主键重复";
		map_order_error[AT_ERROR_CTP_CANNOT_FIND_PK] = "CTP:找不到主键";
		map_order_error[AT_ERROR_CTP_CAN_NOT_INACTIVE_BROKER] = "CTP:设置经纪公司不活跃状态失败";
		map_order_error[AT_ERROR_CTP_BROKER_SYNCHRONIZING] = "CTP:经纪公司正在同步";
		map_order_error[AT_ERROR_CTP_BROKER_SYNCHRONIZED] = "CTP:经纪公司已同步";
		map_order_error[AT_ERROR_CTP_SHORT_SELL] = "CTP:现货交易不能卖空";
		map_order_error[AT_ERROR_CTP_INVALID_SETTLEMENT_REF] = "CTP:不合法的结算引用";
		map_order_error[AT_ERROR_CTP_CFFEX_NETWORK_ERROR] = "CTP:交易所网络连接失败";
		map_order_error[AT_ERROR_CTP_CFFEX_OVER_REQUEST] = "CTP:交易所未处理请求超过许可数";
		map_order_error[AT_ERROR_CTP_CFFEX_OVER_REQUEST_PER_SECOND] = "CTP:交易所每秒发送请求数超过许可数";
		map_order_error[AT_ERROR_CTP_SETTLEMENT_INFO_NOT_CONFIRMED] = "CTP:结算结果未确认";
		map_order_error[AT_ERROR_CTP_DEPOSIT_NOT_FOUND] = "CTP:没有对应的入金记录";
		map_order_error[AT_ERROR_CTP_EXCHANG_TRADING] = "CTP:交易所已经进入连续交易状态";
		map_order_error[AT_ERROR_CTP_PARKEDORDER_NOT_FOUND] = "CTP:找不到预埋（撤单）单";
		map_order_error[AT_ERROR_CTP_PARKEDORDER_HASSENDED] = "CTP:预埋（撤单）单已经发送";
		map_order_error[AT_ERROR_CTP_PARKEDORDER_HASDELETE] = "CTP:预埋（撤单）单已经删除";
		map_order_error[AT_ERROR_CTP_INVALID_INVESTORIDORPASSWORD] = "CTP:无效的投资者或者密码";
		map_order_error[AT_ERROR_CTP_INVALID_LOGIN_IPADDRESS] = "CTP:不合法的登录IP地址";
		map_order_error[AT_ERROR_CTP_OVER_CLOSETODAY_POSITION] = "CTP:平今仓位不足";
		map_order_error[AT_ERROR_CTP_OVER_CLOSEYESTERDAY_POSITION] = "CTP:平昨仓位不足";
		map_order_error[AT_ERROR_CTP_BROKER_NOT_ENOUGH_CONDORDER] = "CTP:经纪公司没有足够可用的条件单数量";
		map_order_error[AT_ERROR_CTP_INVESTOR_NOT_ENOUGH_CONDORDER] = "CTP:投资者没有足够可用的条件单数量";
		map_order_error[AT_ERROR_CTP_BROKER_NOT_SUPPORT_CONDORDER] = "CTP:经纪公司不支持条件单";
		map_order_error[AT_ERROR_CTP_RESEND_ORDER_BROKERINVESTOR_NOTMATCH] = "CTP:重发未知单经济公司/投资者不匹配";
		map_order_error[AT_ERROR_CTP_SYC_OTP_FAILED] = "CTP:同步动态令牌失败";
		map_order_error[AT_ERROR_CTP_OTP_MISMATCH] = "CTP:动态令牌校验错误";
		map_order_error[AT_ERROR_CTP_OTPPARAM_NOT_FOUND] = "CTP:找不到动态令牌配置信息";
		map_order_error[AT_ERROR_CTP_UNSUPPORTED_OTPTYPE] = "CTP:不支持的动态令牌类型";
		map_order_error[AT_ERROR_CTP_SINGLEUSERSESSION_EXCEED_LIMIT] = "CTP:用户在线会话超出上限";
		map_order_error[AT_ERROR_CTP_EXCHANGE_UNSUPPORTED_ARBITRAGE] = "CTP:该交易所不支持套利类型报单";
		map_order_error[AT_ERROR_CTP_NO_CONDITIONAL_ORDER_RIGHT] = "CTP:没有条件单交易权限";
		map_order_error[AT_ERROR_CTP_AUTH_FAILED] = "CTP:客户端认证失败";
		map_order_error[AT_ERROR_CTP_NOT_AUTHENT] = "CTP:客户端未认证";
		map_order_error[AT_ERROR_CTP_SWAPORDER_UNSUPPORTED] = "CTP:该合约不支持互换类型报单";
		map_order_error[AT_ERROR_CTP_OPTIONS_ONLY_SUPPORT_SPEC] = "CTP:该期权合约只支持投机类型报单";
		map_order_error[AT_ERROR_CTP_DUPLICATE_EXECORDER_REF] = "CTP:执行宣告错误，不允许重复执行";
		map_order_error[AT_ERROR_CTP_RESEND_EXECORDER_BROKERINVESTOR_NOTMATCH] = "CTP:重发未知执行宣告经纪公司/投资者不匹配";
		map_order_error[AT_ERROR_CTP_EXECORDER_NOTOPTIONS] = "CTP:只有期权合约可执行";
		map_order_error[AT_ERROR_CTP_OPTIONS_NOT_SUPPORT_EXEC] = "CTP:该期权合约不支持执行";
		map_order_error[AT_ERROR_CTP_BAD_EXECORDER_ACTION_FIELD] = "CTP:执行宣告字段有误";
		map_order_error[AT_ERROR_CTP_DUPLICATE_EXECORDER_ACTION_REF] = "CTP:执行宣告撤单已报送，不允许重复撤单";
		map_order_error[AT_ERROR_CTP_EXECORDER_NOT_FOUND] = "CTP:执行宣告撤单找不到相应执行宣告";
		map_order_error[AT_ERROR_CTP_OVER_EXECUTE_POSITION] = "CTP:执行仓位不足";
		map_order_error[AT_ERROR_CTP_LOGIN_FORBIDDEN] = "CTP:连续登录失败次数超限，登录被禁止";
		map_order_error[AT_ERROR_CTP_INVALID_TRANSFER_AGENT] = "CTP:非法银期代理关系";
		map_order_error[AT_ERROR_CTP_NO_FOUND_FUNCTION] = "CTP:无此功能";
		map_order_error[AT_ERROR_CTP_SEND_EXCHANGEORDER_FAILED] = "CTP:发送报单失败";
		map_order_error[AT_ERROR_CTP_SEND_EXCHANGEORDERACTION_FAILED] = "CTP:发送报单操作失败";
		map_order_error[AT_ERROR_CTP_PRICETYPE_NOTSUPPORT_BYEXCHANGE] = "CTP:交易所不支持的价格类型";
		map_order_error[AT_ERROR_CTP_BAD_EXECUTE_TYPE] = "CTP:错误的执行类型";
		map_order_error[AT_ERROR_CTP_BAD_OPTION_INSTR] = "CTP:无效的组合合约";
		map_order_error[AT_ERROR_CTP_INSTR_NOTSUPPORT_FORQUOTE] = "CTP:该合约不支持询价";
		map_order_error[AT_ERROR_CTP_RESEND_QUOTE_BROKERINVESTOR_NOTMATCH] = "CTP:重发未知报价经纪公司/投资者不匹配";
		map_order_error[AT_ERROR_CTP_INSTR_NOTSUPPORT_QUOTE] = "CTP:该合约不支持报价";
		map_order_error[AT_ERROR_CTP_QUOTE_NOT_FOUND] = "CTP:报价撤单找不到相应报价";
		map_order_error[AT_ERROR_CTP_OPTIONS_NOT_SUPPORT_ABANDON] = "CTP:该期权合约不支持放弃执行";
		map_order_error[AT_ERROR_CTP_COMBOPTIONS_SUPPORT_IOC_ONLY] = "CTP:该组合期权合约只支持IOC";
		map_order_error[AT_ERROR_CTP_OPEN_FILE_FAILED] = "CTP:打开文件失败";
		map_order_error[AT_ERROR_CTP_NEED_RETRY] = "CTP:查询未就绪，请稍后重试";
		map_order_error[AT_ERROR_CTP_EXCHANGE_RTNERROR] = "CTP：交易所返回的错误";
		map_order_error[AT_ERROR_CTP_QUOTE_DERIVEDORDER_ACTIONERROR] = "CTP:报价衍生单要等待交易所返回才能撤单";
		map_order_error[AT_ERROR_CTP_INSTRUMENTMAP_NOT_FOUND] = "CTP:找不到组合合约映射";
		map_order_error[AT_ERROR_CTP_NO_TRADING_RIGHT_IN_SEPC_DR] = "CTP:用户在本系统没有报单权限";
		map_order_error[AT_ERROR_CTP_NO_DR_NO] = "CTP:系统缺少灾备标示号";
		map_order_error[AT_ERROR_CTP_SEND_INSTITUTION_CODE_ERROR] = "CTP:银期转账：发送机构代码错误";
		map_order_error[AT_ERROR_CTP_NO_GET_PLATFORM_SN] = "CTP:银期转账：取平台流水号错误";
		map_order_error[AT_ERROR_CTP_ILLEGAL_TRANSFER_BANK] = "CTP:银期转账：不合法的转账银行";
		map_order_error[AT_ERROR_CTP_ALREADY_OPEN_ACCOUNT] = "CTP:银期转账：已经开户";
		map_order_error[AT_ERROR_CTP_NOT_OPEN_ACCOUNT] = "CTP:银期转账：未开户";
		map_order_error[AT_ERROR_CTP_PROCESSING] = "CTP:银期转账：处理中";
		map_order_error[AT_ERROR_CTP_OVERTIME] = "CTP:银期转账：交易超时";
		map_order_error[AT_ERROR_CTP_RECORD_NOT_FOUND] = "CTP:银期转账：找不到记录";
		map_order_error[AT_ERROR_CTP_NO_FOUND_REVERSAL_ORIGINAL_TRANSACTION] = "CTP:银期转账：找不到被冲正的原始交易";
		map_order_error[AT_ERROR_CTP_CONNECT_HOST_FAILED] = "CTP:银期转账：连接主机失败";
		map_order_error[AT_ERROR_CTP_SEND_FAILED] = "CTP:银期转账：发送失败";
		map_order_error[AT_ERROR_CTP_LATE_RESPONSE] = "CTP:银期转账：迟到应答";
		map_order_error[AT_ERROR_CTP_REVERSAL_BANKID_NOT_MATCH] = "CTP:银期转账：冲正交易银行代码错误";
		map_order_error[AT_ERROR_CTP_REVERSAL_BANKACCOUNT_NOT_MATCH] = "CTP:银期转账：冲正交易银行账户错误";
		map_order_error[AT_ERROR_CTP_REVERSAL_BROKERID_NOT_MATCH] = "CTP:银期转账：冲正交易经纪公司代码错误";
		map_order_error[AT_ERROR_CTP_REVERSAL_ACCOUNTID_NOT_MATCH] = "CTP:银期转账：冲正交易资金账户错误";
		map_order_error[AT_ERROR_CTP_REVERSAL_AMOUNT_NOT_MATCH] = "CTP:银期转账：冲正交易交易金额错误";
		map_order_error[AT_ERROR_CTP_DB_OPERATION_FAILED] = "CTP:银期转账：数据库操作错误";
		map_order_error[AT_ERROR_CTP_SEND_ASP_FAILURE] = "CTP:银期转账：发送到交易系统失败";
		map_order_error[AT_ERROR_CTP_NOT_SIGNIN] = "CTP:银期转账：没有签到";
		map_order_error[AT_ERROR_CTP_ALREADY_SIGNIN] = "CTP:银期转账：已经签到";
		map_order_error[AT_ERROR_CTP_AMOUNT_OR_TIMES_OVER] = "CTP:银期转账：金额或次数超限";
		map_order_error[AT_ERROR_CTP_NOT_IN_TRANSFER_TIME] = "CTP:银期转账：这一时间段不能转账";
		map_order_error[AT_ERROR_CTP_BANK_SERVER_ERROR] = "银行主机错";
		map_order_error[AT_ERROR_CTP_BANK_SERIAL_IS_REPEALED] = "CTP:银期转账：银行已经冲正";
		map_order_error[AT_ERROR_CTP_BANK_SERIAL_NOT_EXIST] = "CTP:银期转账：银行流水不存在";
		map_order_error[AT_ERROR_CTP_NOT_ORGAN_MAP] = "CTP:银期转账：机构没有签约";
		map_order_error[AT_ERROR_CTP_EXIST_TRANSFER] = "CTP:银期转账：存在转账，不能销户";
		map_order_error[AT_ERROR_CTP_BANK_FORBID_REVERSAL] = "CTP:银期转账：银行不支持冲正";
		map_order_error[AT_ERROR_CTP_DUP_BANK_SERIAL] = "CTP:银期转账：重复的银行流水";
		map_order_error[AT_ERROR_CTP_FBT_SYSTEM_BUSY] = "CTP:银期转账：转账系统忙，稍后再试";
		map_order_error[AT_ERROR_CTP_MACKEY_SYNCING] = "CTP:银期转账：MAC密钥正在同步";
		map_order_error[AT_ERROR_CTP_ACCOUNTID_ALREADY_REGISTER] = "CTP:银期转账：资金账户已经登记";
		map_order_error[AT_ERROR_CTP_BANKACCOUNT_ALREADY_REGISTER] = "CTP:银期转账：银行账户已经登记";
		map_order_error[AT_ERROR_CTP_DUP_BANK_SERIAL_REDO_OK] = "重发成功";
		map_order_error[AT_ERROR_CTP_CURRENCYID_NOT_SUPPORTED] = "CTP:银期转账：该币种代码不支持";
		map_order_error[AT_ERROR_CTP_INVALID_MAC] = "CTP:银期转账：MAC值验证失败";
		map_order_error[AT_ERROR_CTP_NOT_SUPPORT_SECAGENT_BY_BANK] = "CTP:银期转账：不支持银行端发起的二级代理商转账和查询";
		map_order_error[AT_ERROR_CTP_PINKEY_SYNCING] = "CTP:银期转账：PIN密钥正在同步";
		map_order_error[AT_ERROR_CTP_SECAGENT_QUERY_BY_CCB] = "CTP:银期转账：建行发起的二级代理商查询";
		map_order_error[AT_ERROR_CTP_NO_VALID_BANKOFFER_AVAILABLE] = "CTP:该报盘未连接到银行";
		map_order_error[AT_ERROR_CTP_PASSWORD_MISMATCH] = "CTP:资金密码错误";
		map_order_error[AT_ERROR_CTP_DUPLATION_BANK_SERIAL] = "CTP:银行流水号重复";
		map_order_error[AT_ERROR_CTP_DUPLATION_OFFER_SERIAL] = "CTP:报盘流水号重复";
		map_order_error[AT_ERROR_CTP_SERIAL_NOT_EXSIT] = "CTP:被冲正流水不存在(冲正交易)";
		map_order_error[AT_ERROR_CTP_SERIAL_IS_REPEALED] = "CTP:原流水已冲正(冲正交易)";
		map_order_error[AT_ERROR_CTP_SERIAL_MISMATCH] = "CTP:与原流水信息不符(冲正交易)";
		map_order_error[AT_ERROR_CTP_IdentifiedCardNo_MISMATCH] = "CTP:证件号码或类型错误";
		map_order_error[AT_ERROR_CTP_ACCOUNT_NOT_FUND] = "CTP:资金账户不存在";
		map_order_error[AT_ERROR_CTP_ACCOUNT_NOT_ACTIVE] = "CTP:资金账户已经销户";
		map_order_error[AT_ERROR_CTP_NOT_ALLOW_REPEAL_BYMANUAL] = "CTP:该交易不能执行手工冲正";
		map_order_error[AT_ERROR_CTP_AMOUNT_OUTOFTHEWAY] = "CTP:转帐金额错误";
		map_order_error[AT_ERROR_CTP_EXCHANGERATE_NOT_FOUND] = "CTP:找不到汇率";
		map_order_error[AT_ERROR_CTP_WAITING_OFFER_RSP] = "CTP:等待银期报盘处理结果";
		map_order_error[AT_ERROR_CTP_FBE_NO_GET_PLATFORM_SN] = "CTP:银期换汇：取平台流水号错误";
		map_order_error[AT_ERROR_CTP_FBE_ILLEGAL_TRANSFER_BANK] = "CTP:银期换汇：不合法的转账银行";
		map_order_error[AT_ERROR_CTP_FBE_PROCESSING] = "CTP:银期换汇：处理中";
		map_order_error[AT_ERROR_CTP_FBE_OVERTIME] = "CTP:银期换汇：交易超时";
		map_order_error[AT_ERROR_CTP_FBE_RECORD_NOT_FOUND] = "CTP:银期换汇：找不到记录";
		map_order_error[AT_ERROR_CTP_FBE_CONNECT_HOST_FAILED] = "CTP:银期换汇：连接主机失败";
		map_order_error[AT_ERROR_CTP_FBE_SEND_FAILED] = "CTP:银期换汇：发送失败";
		map_order_error[AT_ERROR_CTP_FBE_LATE_RESPONSE] = "CTP:银期换汇：迟到应答";
		map_order_error[AT_ERROR_CTP_FBE_DB_OPERATION_FAILED] = "CTP:银期换汇：数据库操作错误";
		map_order_error[AT_ERROR_CTP_FBE_NOT_SIGNIN] = "CTP:银期换汇：没有签到";
		map_order_error[AT_ERROR_CTP_FBE_ALREADY_SIGNIN] = "CTP:银期换汇：已经签到";
		map_order_error[AT_ERROR_CTP_FBE_AMOUNT_OR_TIMES_OVER] = "CTP:银期换汇：金额或次数超限";
		map_order_error[AT_ERROR_CTP_FBE_NOT_IN_TRANSFER_TIME] = "CTP:银期换汇：这一时间段不能换汇";
		map_order_error[AT_ERROR_CTP_FBE_BANK_SERVER_ERROR] = "CTP:银期换汇：银行主机错";
		map_order_error[AT_ERROR_CTP_FBE_NOT_ORGAN_MAP] = "CTP:银期换汇：机构没有签约";
		map_order_error[AT_ERROR_CTP_FBE_SYSTEM_BUSY] = "CTP:银期换汇：换汇系统忙，稍后再试";
		map_order_error[AT_ERROR_CTP_FBE_CURRENCYID_NOT_SUPPORTED] = "CTP:银期换汇：该币种代码不支持";
		map_order_error[AT_ERROR_CTP_FBE_WRONG_BANK_ACCOUNT] = "CTP:银期换汇：银行帐号不正确";
		map_order_error[AT_ERROR_CTP_FBE_BANK_ACCOUNT_NO_FUNDS] = "CTP:银期换汇：银行帐户余额不足";
		map_order_error[AT_ERROR_CTP_FBE_DUP_CERT_NO] = "CTP:银期换汇：凭证号重复";

		init = true;
	}

	char error_msg[256] = {0};
	if (server_error != NULL && strlen(server_error) != 0) {
		if (map_server_error.find(server_error) != map_server_error.end()) {
			sprintf(error_msg, "%s - %s", server_error, map_server_error[server_error].c_str());
		}
		else {
			sprintf(error_msg, "%s", server_error);
		}
	}
	else {
		if (map_order_error.find(error_code) != map_order_error.end()) {
			sprintf(error_msg, "%s", map_order_error[error_code].c_str());
		}
		else {
			sprintf(error_msg, "%s", map_order_error[AT_ERROR_UNSPECIFIED].c_str());
		}
	}
	return error_msg;
}

}
