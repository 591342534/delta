/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: trade_server.cpp
 Version: 1.0
 Date: 2016.03.21

 History:
 david wang     2016.03.21   1.0     Create
 ******************************************************************************/

#include "base/aes.h"
#include "base/pugixml.hpp"
#include "base/trace.h"
#include "base/base.h"
#include "base/util.h"
#include "base/dictionary.h"
#include "base/reference_base.h"
#include "base/dtrans.h"
#include "database/unidb.h"
#include "trade_server.h"
#include "common.h"
#include "version.h"
#include <iostream>
namespace ctp
{

trade_server::trade_server()
	: localno_(time(NULL))
	, stop_rsp_thread_(false)
    , m_sptr_rsp_thread(nullptr)
	, rsp_queue_(NULL)
	, rsp_event_(NULL)
	, started_(false)

{
    map_accounts_info_.clear();
    map_tunits_.clear();
}

trade_server::~trade_server()
{
	stop();
}

int trade_server::start(const char* config_file)
{
	TRACE_SYSTEM(AT_TRACE_TAG, "--start trader server --");

	if (started_) {
		return NAUT_AT_S_OK;
	}

	int ret = load_config(config_file);
	CHECK_LABEL(ret, end);

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
	TRACE_SYSTEM(AT_TRACE_TAG, "-- stop trader server --");

	started_ = false;
	return stop_internal();
}

int trade_server::start_internal()
{
    int ret = NAUT_AT_S_OK;

    LABEL_SCOPE_START;

    ret = init_db_pool();
    CHECK_LABEL(ret, end);

    {
        database::dbscope mysql_db_keepper(m_db_pool_);
        database::db_instance* dbconn = mysql_db_keepper.get_db_conn();
        CHECK_IF_DBCONN_NULL(dbconn);
        m_comm_holiday_.load_holiday(dbconn, params_.m_statutory_holiday_tbl_name_);
    }

    ret = request_account_config();
    CHECK_LABEL(ret, end);

    /* initialize localno */
    ret = init_localno();
    CHECK_LABEL(ret, end);


    /* initialize trade units */
    for (int i = 0; i < (int)params_.ar_accounts_info.size(); i++)
    {
        trade_unit_params tparams;
        tparams.trade_host = params_.m_server_config.host;
        tparams.trade_port = params_.m_server_config.port;
        tparams.userid = params_.ar_accounts_info[i].userid;
        tparams.password = params_.ar_accounts_info[i].password;
        tparams.account = params_.ar_accounts_info[i].account;
        tparams.broker = params_.ar_accounts_info[i].broker;
        tparams.use_simulation_flag = false;
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

    /* initialize mq response references */
    rsp_queue_ = new base::srt_queue<atp_message>(10);
    rsp_queue_->init();
    rsp_event_ = new base::event();

    /* start response process thread */
    stop_rsp_thread_ = false;
    m_sptr_rsp_thread = std::make_shared<std::thread>
        (process_rsp_thread, this);

    LABEL_SCOPE_END;

end:
    return ret;
}

int trade_server::stop_internal()
{
    /* stop mq response thread */
    if (m_sptr_rsp_thread != nullptr) {
        stop_rsp_thread_ = true;
        m_sptr_rsp_thread->join();
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

    return NAUT_AT_S_OK;
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
	case ATP_MESSAGE_TYPE_ORDER_REPLY:
    case ATP_MESSAGE_TYPE_QUERY_REPLY:
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
	default:
		TRACE_WARNING(AT_TRACE_TAG, "unknown message type: %d", msg.type);
		break;
	}
	return NAUT_AT_S_OK;
}


int trade_server::load_config(const char* config_file)
{
	assert(config_file != NULL);

	int ret = NAUT_AT_S_OK;

	LABEL_SCOPE_START;

	pugi::xml_document doc;
	if (!doc.load_file(config_file)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIGFILE_INVALID,
				"ctp_trade: config file is not exist or invalid '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIGFILE_INVALID, end);
	}

	pugi::xml_node xroot = doc.child("ctp_trade");
	if (xroot.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp_trade: root element 'ctp_trade' should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}

	/* server name */
	std::string tmp = xroot.child("server_name").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp_trade: server name should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
	params_.server_name = tmp;

	/* server config database */
	pugi::xml_node xserver_config_database = xroot.child("server_config_database");
	if (xserver_config_database.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp_trade: server config database should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}

	tmp = xserver_config_database.child("host").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp_trade: host of the server config database should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
    params_.server_config_database.host = tmp;

	tmp = xserver_config_database.child("port").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp_trade: port of the server config database should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
	params_.server_config_database.port = atoi(tmp.c_str());

	tmp = xserver_config_database.child("user").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp_trade: user of the server config database should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
	params_.server_config_database.user = tmp;

	tmp = xserver_config_database.child("password").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp_trade: password of the server config database should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
	params_.server_config_database.password = tmp;

    tmp = xserver_config_database.child("dbname").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
            "ctp_trade: dbname of the server config database should be specified, '%s'", config_file);
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
    }
    params_.server_config_database.dbname = tmp;

	/* processor_config */
	pugi::xml_node xprocessor_config = xroot.child("processor_config");
	if (xprocessor_config.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp_trade: element 'processor_config' should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}

	tmp = xprocessor_config.child("query_thread_count").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp_trade: query_thread_count should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
	params_.query_thread_count = atoi(tmp.c_str());

	tmp = xprocessor_config.child("trade_thread_count").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
				"ctp_trade: trade_thread_count should be specified, '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
	}
	params_.trade_thread_count = atoi(tmp.c_str());

   tmp = xprocessor_config.child("unit_thread_count").text().as_string();
   if (tmp.empty()) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
                "ctp_trade: unit_thread_count should be specified, '%s'", config_file);
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
    }
    params_.unit_thread_count = atoi(tmp.c_str());

	/* blog root path */
	tmp = xroot.child("blog_root_path").text().as_string();
	if (!tmp.empty()) {
		params_.blog_root_path = tmp;
		if (params_.blog_root_path.rfind("/") != params_.blog_root_path.length() - 1) {
			params_.blog_root_path += "/";
		}
	}

	/* switch time */
	tmp = xroot.child("switch_time").text().as_string();
	if (!tmp.empty()) {
		bool valid = false;
		int hour, minute, second;
		if (sscanf(tmp.c_str(), "%02d:%02d:%02d", &hour, &minute, &second) != 0) {
			if ((hour >= 0 && hour < 24) && (minute >= 0 && minute < 60) && (second >= 0 && second < 60)) {
				valid = true;
			}
		}
		if (valid) {
			params_.switch_time = tmp;
		}
		else {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
					"ctp_trade: format of switch time is invalid, '%s'", tmp.c_str());
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
		}
	}

    /* check_interval path */
    int intvalue = xroot.child("check_interval").text().as_int();
    if (intvalue != 0) {
        params_.check_interval_ = intvalue;
    }

    /* table_config */
    pugi::xml_node xtable_config = xroot.child("table_config");
    if (xtable_config.empty()) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
            "ctp_trade: element 'table_config' should be specified, '%s'", config_file);
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
    }

    tmp = xtable_config.child("account").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
            "ctp_trade: account should be specified, '%s'", config_file);
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
    }
    params_.m_account_tbl_name_ = tmp;

    tmp = xtable_config.child("statutory_holidays").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
            "ctp_trade: statutory_holidays should be specified, '%s'", config_file);
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
    }
    params_.m_statutory_holiday_tbl_name_ = tmp;

    tmp = xtable_config.child("entrust").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
            "ctp_trade: entrust should be specified, '%s'", config_file);
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
    }
    params_.m_entrust_tbl_name_ = tmp;

    tmp = xtable_config.child("deal").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
            "ctp_trade: deal should be specified, '%s'", config_file);
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
    }
    params_.m_deal_tbl_name_ = tmp;

    /* ctp_server_info */
    pugi::xml_node xctp_server_info = xroot.child("ctp_server_info");
    if (xtable_config.empty()) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
            "ctp_trade: element 'ctp_server_info' should be specified, '%s'", config_file);
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
    }

    tmp = xctp_server_info.child("ip").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONFIG_INVALID,
            "ctp_trade: ip should be specified, '%s'", config_file);
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONFIG_INVALID, end);
    }
    params_.m_server_config.host = tmp;

    int port = xctp_server_info.child("port").text().as_int();
    if (port != 0) {
        params_.m_server_config.port = port;
    }

	LABEL_SCOPE_END;

end:
	return ret;
}

int trade_server::init_db_pool()
{
    int ret = NAUT_AT_S_OK;

    LABEL_SCOPE_START;
    m_db_pool_.release_all_conns();
    // pub_trade_db_pool_ initialize database
    database::unidb_param dbparam_pub;
    dbparam_pub.create_database_if_not_exists = false;
    dbparam_pub.recreate_database_if_exists = false;
    dbparam_pub.host = params_.server_config_database.host;
    dbparam_pub.port = params_.server_config_database.port;
    dbparam_pub.user = params_.server_config_database.user;
    dbparam_pub.password = params_.server_config_database.password;
    dbparam_pub.database_name = params_.server_config_database.dbname;

    std::string str_db_type = "mysql";
    int conn_count = 1;
    ret = m_db_pool_.init(dbparam_pub, conn_count, str_db_type, params_.switch_time);
    CHECK_LABEL(ret, end);

    LABEL_SCOPE_END;

end:
    return ret;
}

int trade_server::request_account_config()
{
    int ret = NAUT_AT_S_OK;

    LABEL_SCOPE_START;

    database::dbscope mysql_db_keepper(m_db_pool_);
    database::db_instance* dbconn = mysql_db_keepper.get_db_conn();
    CHECK_IF_DBCONN_NULL(dbconn);

    char sql[512];
    sprintf(sql, "select userid, password, account, broker from %s "
        "where server_name='%s'", params_.m_account_tbl_name_.c_str(),
        params_.server_name.c_str());
    if (!dbconn->_conn->query(sql)) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_QUERY_ACCOUNT_CONFIG_FAILED,
            "query account config failed, db error: (%d:%s)", dbconn->_conn->get_errno(),
            dbconn->_conn->get_error().c_str());
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_QUERY_ACCOUNT_CONFIG_FAILED, end);
    }

    if (dbconn->_conn->get_count() == 0) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_CONFIG_NOT_EXIT,
            "account config of the specified server_name is not exist");
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNT_CONFIG_NOT_EXIT, end);
    }

    while (dbconn->_conn->fetch_row()) {
        trade_account_info account_info;
        account_info.userid = dbconn->_conn->get_string("userid");
        if (account_info.userid.empty()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_CONFIG_INVALID,
                "account-config: userid of the account should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNT_CONFIG_INVALID, end);
        }

        std::string tmp = dbconn->_conn->get_string("password");
        account_info.password = tmp;
        if (account_info.password.empty()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_CONFIG_INVALID,
                "account-config: password of the account should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNT_CONFIG_INVALID, end);
        }

        account_info.account = dbconn->_conn->get_string("account");
        if (account_info.account.empty()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_CONFIG_INVALID,
                "account-config: capital account of the account should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNT_CONFIG_INVALID, end);
        }

        account_info.broker = dbconn->_conn->get_string("broker");
        if (account_info.broker.empty()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ACCOUNT_CONFIG_INVALID,
                "account-config: broker of the account should be specified");
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ACCOUNT_CONFIG_INVALID, end);
        }
        params_.ar_accounts_info.push_back(account_info);
    }

    LABEL_SCOPE_END;

end:
    if (BFAILED(ret)) {
        params_.ar_accounts_info.clear();
    }
    return ret;
}


int trade_server::init_localno()
{
	int ret = NAUT_AT_S_OK;
    database::dbscope mysql_db_keepper(m_db_pool_);
    database::db_instance* dbconn = mysql_db_keepper.get_db_conn();
    CHECK_IF_DBCONN_NULL(dbconn);

    LABEL_SCOPE_START;

	char sql[2048];
	sprintf(sql, "select max(localno) as max_localno from %s",
        params_.m_entrust_tbl_name_.c_str());
	if (!dbconn->_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
				"select max localno from database failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
	}

	if (dbconn->_conn->fetch_row()) {
		localno_ = (int)dbconn->_conn->get_long("max_localno") + 10000;
	}
	else {
		localno_ = 1;
	}

	LABEL_SCOPE_END;

end:
	return ret;
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
        if (!rsp_queue_->pop(msg)) {
            rsp_event_->reset();
            rsp_event_->wait(20);
            continue;
        }

        ref_dictionary* rd = (ref_dictionary*)msg.param1;
        assert(rd != NULL);

        TRACE_SYSTEM(AT_TRACE_TAG, "send response: %s", rd->get()->to_string().c_str());

        switch (msg.type) {
        case ATP_MESSAGE_TYPE_DEAL_PUSH:
        case ATP_MESSAGE_TYPE_ORDER_REPLY:
        {
                                             std::string result = base::djson::dict2str(rd->get());
                                             std::cout << result << endl;
        }
            break;
        case ATP_MESSAGE_TYPE_QUERY_REPLY:
        {
                                             std::string result = base::djson::dict2str(rd->get());
                                             std::cout << result << endl;
        }
            break;
        default:
            break;
        }
        rd->release();
	}
}

void trade_server::process_rsp_thread(void* param)
{
	trade_server* ts = (trade_server*)param;
	ts->process_rsp();
}

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
