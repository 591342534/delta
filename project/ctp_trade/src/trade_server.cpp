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
	case ATP_MESSAGE_TYPE_SERVER_QUERY_RSP:
    case ATP_MESSAGE_TYPE_CHECKER_REQ:
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
    case ATP_MESSAGE_TYPE_ORDER_REPLY:
    case ATP_MESSAGE_TYPE_QUERY_REPLY:
    case ATP_MESSAGE_TYPE_DEAL_PUSH:
        post_rsp_message(msg);
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
