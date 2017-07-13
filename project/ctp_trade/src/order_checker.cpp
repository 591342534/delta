/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: deal_checker.cpp
 Version: 1.0
 Date: 2016.03.21

 History:
 david wang     2016.03.21   1.0     Create
 ******************************************************************************/

#include "order_checker.h"
#include "common.h"
#include "trade_server.h"
#include "base/trace.h"
#include "base/util.h"

namespace ctp
{

order_checker::order_checker()
	: mdpt_(NULL)
	, msg_event_(NULL)
	, last_check_time_(0)
	, check_interval_(10000)
	, check_flag_(0)
	, check_id_(0)
	, wait_timeout_(QUERY_WAIT_TIMEOUT)
	, started_(false)
{

}

order_checker::~order_checker()
{
	stop();
}

int order_checker::start(const char* server_name,
		message_dispatcher* mdpt, trade_server* ts)
{
	assert(mdpt != NULL);
	tserver_ = ts;
	if (started_) {
		return NAUT_AT_S_OK;
	}

	server_name_ = server_name;
	mdpt_ = mdpt;
	check_interval_ = tserver_->server_param().check_interval_ * 1000;

	int ret = start_internal();
	if (BSUCCEEDED(ret)) {
		started_ = true;
	}
	else {
		stop();
	}
	return ret;
}

void order_checker::post(const atp_message& msg)
{
	msg_queue_->push(msg);

	if (started_) {
		assert(msg_event_ != NULL);
		msg_event_->set();
	}
}

void order_checker::stop()
{
	started_ = false;
	stop_internal();
}

void order_checker::start()
{
	processor_base::start();
}

int order_checker::start_internal()
{
	int ret = NAUT_AT_S_OK;

    naut::DBCONNECT* db_conn = NULL;

	LABEL_SCOPE_START;

	msg_event_ = new base::event();

	/* allocate a database connection used in the processor */
    db_conn = tserver_->get_conn_pool()->getconn();
	if (db_conn == NULL) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_INIT_DATABASE_CONN_FAILED,
				"order checker processor initialize database connection failed");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_INIT_DATABASE_CONN_FAILED, end);
	}

	/* get query wait timeout parameter */
//	char sql[1024];
//	sprintf(sql, "select * from trade_product where param_code='%d'",
//			PARAM_CODE_CHECKER_WAIT_TIMEOUT);
//	if (!db_conn->_conn->query(sql)) {
//		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
//				"query order checker wait timeout parameter failed");
//		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
//	}
//
//	if (db_conn->_conn->fetch_row()) {
//		wait_timeout_ = db_conn->_conn->get_long("param_value") * 1000;
//	}
	wait_timeout_ = 120;
	last_check_time_ = base::util::clock() + check_interval_;

	/* start process thread */
	processor_base::start();

	LABEL_SCOPE_END;

end:
    if (db_conn != NULL) {
        tserver_->get_conn_pool()->retconn(db_conn);
    }
	return ret;
}

int order_checker::stop_internal()
{
	/* stop process thread */
	processor_base::stop();

	/* release trade messages left in the queue */
	release_messages();

	if (msg_event_ != NULL) {
		delete msg_event_;
		msg_event_ = NULL;
	}

	return NAUT_AT_S_OK;
}

void order_checker::run()
{
	atp_message msg;
	while (is_running_)
	{
        if(!tserver_->get_server_start_flag()) {
            base::util::sleep(1000);
            continue;
        }

		long cur_time = base::util::clock();
		if (cur_time > last_check_time_ && msg_queue_->size() == 0) {
			if (check_flag_ == 0) {
				check_entrust();
				check_flag_ = 1;
			}
			else {
				check_deal();
				check_flag_ = 0;
			}
			last_check_time_ = cur_time + check_interval_;
		}

		int ret = get(msg);
		if (ret != 0) {
			msg_event_->reset();
			msg_event_->wait(100);
			continue;
		}

		ref_dictionary* rd = (ref_dictionary*)msg.param1;
		base::dictionary* dict = rd->get();
		std::string cmd = (*dict)["cmd"].string_value();

		if (cmd == ATPM_CMD_RSP_CHECKER_DEAL) {
			process_rsp_checker_deal(*dict);
		}
		else if (cmd == ATPM_CMD_RSP_CHECKER_ENTRUST) {
			process_rsp_checker_entrust(*dict);
		}

		rd->release();
	}
}

std::string order_checker::get_check_id()
{
	check_id_++;
	char id[64];
	sprintf(id, "order_checker_%d", check_id_);
	return id;
}

int order_checker::check_deal()
{
	int ret = NAUT_AT_S_OK;

    naut::DBCONNECT* db_conn = NULL;

    LABEL_SCOPE_START;

	/* erase deal if timeout */
	VBASE_HASH_MAP<std::string, long>::iterator mit = map_deal_timeout_.begin();
	for (; mit != map_deal_timeout_.end(); ) {
		if (mit->second < base::util::clock()) {
			mit = map_deal_timeout_.erase(mit);
		}
		else {
			mit++;
		}
	}

	/* select undealed orders, query the entrust result if we don't get deal in 120 seconds */
	char sql[2048];
	sprintf(sql, "select * from entrust where"
			" server_name='%s' and order_status<>%d and order_status<>%d and order_status<>%d"
			" and error_code=0 and abs(TIME_TO_SEC(TIMEDIFF(record_time, CURTIME()))) > 120",
			server_name_.c_str(), ORDER_STATUS_DEAL_ALL, ORDER_STATUS_WITHDRAWED, ORDER_STATUS_FAILED);

    db_conn = tserver_->get_conn_pool()->getconn();
	if (db_conn == NULL || !db_conn->_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
				"select incomplete orderid from entrust failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
	}

	while (db_conn->_conn->fetch_row())
	{
		std::string systemno = db_conn->_conn->get_string("systemno");
		if (systemno.empty()) {
			continue;
		}

		/* set timeout */
		std::string account = db_conn->_conn->get_string("account");
		std::string key = account + "_" + systemno;
		if (map_deal_timeout_.find(key) != map_deal_timeout_.end()) {
			if (map_deal_timeout_[key] > base::util::clock()) {
				continue;
			}
		}
		map_deal_timeout_[key] = base::util::clock() + wait_timeout_;

		/* send deal check request */
		base::dictionary* dict = new base::dictionary();
		ref_dictionary* rd = new ref_dictionary(dict);

		(*dict)["cmd"] = ATPM_CMD_QRY_CHECKER_DEAL;
		(*dict)["queryid"] = get_check_id();
		(*dict)["userid"] = db_conn->_conn->get_string("userid");
		(*dict)["account"] = account;
		(*dict)["systemno"] = systemno;
		(*dict)["orderid"] = db_conn->_conn->get_string("orderid");
		(*dict)["broker"] = db_conn->_conn->get_string("broker");
		(*dict)["entrust_type"] = db_conn->_conn->get_string("entrust_type");
		(*dict)["direction"] = db_conn->_conn->get_string("direction");
		(*dict)["order_type"] = db_conn->_conn->get_long("order_type");
		(*dict)["excode"] = db_conn->_conn->get_string("excode");
		(*dict)["code"] = db_conn->_conn->get_string("code");
		(*dict)["localno"] = db_conn->_conn->get_string("localno");

		TRACE_DEBUG(AT_TRACE_TAG, "dict: %s", dict->to_string().c_str());

		atp_message msg;
		msg.type = ATP_MESSAGE_TYPE_CHECKER_REQ;
		msg.param1 = (void*)rd;

		mdpt_->dispatch_message(msg);

//		base::util::sleep(15);
	}

	LABEL_SCOPE_END;

end:
    if (db_conn != NULL) {
        tserver_->get_conn_pool()->retconn(db_conn);
    }
	return ret;
}

int order_checker::process_rsp_checker_deal(base::dictionary& dict)
{
	if (dict["error_code"].int_value() != 0) {
		return NAUT_AT_S_OK;
	}
	int count = dict["count"].int_value();
	for (int i = 0; i < count; i++) {
		base::darray* da = dict["result"].array_value();
		process_rsp_checker_single_deal(*(da->at(i).dictionary_value()));
	}
	return NAUT_AT_S_OK;
}

int order_checker::process_rsp_checker_single_deal(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;
	naut::DBCONNECT* db_conn = NULL;

	LABEL_SCOPE_START;
	db_conn = tserver_->get_conn_pool()->getconn();
	/* check whether the deal has existed */
	char sql[2048];
	sprintf(sql, "select dealno from deal where dealno='%s' and deal_date='%s' and account='%s' and broker='%s' and entrustno='%s'",
			dict["dealno"].string_value().c_str(),
			dict["deal_date"].string_value().c_str(),
			dict["account"].string_value().c_str(),
			dict["broker"].string_value().c_str(),
			dict["entrustno"].string_value().c_str());
	if (db_conn == NULL || !db_conn->_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
				"select and check dealno from deal failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
	}
	bool deal_existed = false;
	if (db_conn->_conn->get_count() > 0) {
		TRACE_WARNING(AT_TRACE_TAG, "dealno has already existed, sql: '%s'", sql);
		deal_existed = true;
	}

	/* check whether the order exists in table 'entrust' */
	sprintf(sql, "select orderid, entrustid, direction, entrust_type, entrust_amount"
			" from entrust where account='%s' and orderid='%s'",
			dict["account"].string_value().c_str(),
			dict["orderid"].string_value().c_str());
	if (!db_conn->_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
				"select and check order from entrust failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
	}

	if (db_conn->_conn->get_count() != 1) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ENTRUST_CHECK_ERROR,
				"entrust is not exist or not unique, account: %s, localno: %s, count: %d",
				dict["account"].string_value().c_str(), dict["localno"].string_value().c_str(),
				db_conn->_conn->get_count());
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ENTRUST_CHECK_ERROR, end);
	}

	db_conn->_conn->fetch_row();
	std::string orderid = db_conn->_conn->get_string("orderid");
	std::string entrustid = db_conn->_conn->get_string("entrustid");
	std::string systemno = db_conn->_conn->get_string("systemno");
	int entrust_amount = db_conn->_conn->get_long("entrust_amount");

	int at_error = dict["at_error"].int_value();
	std::string error_msg = get_response_error_msg(at_error);;
	if (at_error >= AT_ERROR_TRADER) {
	    at_error = AT_ERROR_TRADER;
	}

	/* insert deal record to table */
	if (!deal_existed) {
	    sprintf(sql, "insert into deal (server_name, entrustid, entrustno, dealno, orderid, broker,"
	            " userid, account, code, excode, deal_amount, deal_price, deal_fund, deal_date,"
	            " deal_time, error_code, error_msg, record_time, direction, entrust_type, code_name) values ('%s', '%s', '%s', '%s',"
	            " '%s', '%s', '%s', '%s', '%s', '%s', %d, %.3f, %.3lf, '%s', '%s', %d, '%s',CURTIME(), %d, %d, '%s')",
	            server_name_.c_str(), entrustid.c_str(), dict["entrustno"].string_value().c_str(),
	            dict["dealno"].string_value().c_str(), orderid.c_str(), dict["broker"].string_value().c_str(),
	            dict["userid"].string_value().c_str(), dict["account"].string_value().c_str(),
	            dict["code"].string_value().c_str(), dict["excode"].string_value().c_str(),
	            dict["deal_amount"].int_value(), dict["deal_price"].double_value(),
	            dict["deal_fund"].double_value(), dict["deal_date"].string_value().c_str(),
	            dict["deal_time"].string_value().c_str(), at_error, error_msg.c_str(),
	            dict["direction"].int_value(), dict["entrust_type"].int_value(), dict["code_name"].string_value().c_str());


		if (!db_conn->_conn->execute(sql)) {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
				"insert record into deal failed, sql: '%s'", sql);
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_FAILED, end);
		}
	}

	/* update entrust order status and report time */
	if (at_error == AT_ERROR_NONE) {
		int order_status = ORDER_STATUS_DEAL;
		if (entrust_amount != 0 && dict["deal_amount"].int_value() == entrust_amount) {
			order_status = ORDER_STATUS_DEAL_ALL;
		}
		else {
			/* if there exist multiple deal records for the entrust, check the sum */
			sprintf(sql, "select sum(deal_amount) as total_deal_amount from"
					" deal where entrustno='%s' and account='%s'",
					dict["entrustno"].string_value().c_str(),
					dict["account"].string_value().c_str());
			if (!db_conn->_conn->query(sql)) {
				TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
						"select sum(deal_amount) failed, sql: '%s'", sql);
			}
			else {
				if (db_conn->_conn->fetch_row()) {
					int total_deal_amount = db_conn->_conn->get_long("total_deal_amount");
					if (total_deal_amount == entrust_amount) {
						order_status = ORDER_STATUS_DEAL_ALL;
					}
				}
			}
		}
		if (systemno.empty()) {
			systemno = dict["systemno"].string_value();
		}
		sprintf(sql, "update entrust set order_status=%d, report_time='%s',"
				" entrustno='%s', systemno='%s' where account='%s' and localno='%s'",
				order_status, dict["deal_time"].string_value().c_str(),
				dict["entrustno"].string_value().c_str(), systemno.c_str(),
				dict["account"].string_value().c_str(),
				dict["localno"].string_value().c_str());
		if (!db_conn->_conn->execute(sql)) {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
					"update entrust order status failed, sql: '%s'", sql);
		}
	}

	LABEL_SCOPE_END;

end:
    if (db_conn != NULL) {
        tserver_->get_conn_pool()->retconn(db_conn);
    }
	return ret;
}

int order_checker::check_entrust()
{
	int ret = NAUT_AT_S_OK;
    naut::DBCONNECT* db_conn = NULL;

    LABEL_SCOPE_START;
    db_conn = tserver_->get_conn_pool()->getconn();

	/* erase entrust if timeout */
	VBASE_HASH_MAP<std::string, long>::iterator mit = map_entrust_timeout_.begin();
	for (; mit != map_entrust_timeout_.end(); ) {
		if (mit->second < base::util::clock()) {
			mit = map_entrust_timeout_.erase(mit);
		}
		else {
			mit++;
		}
	}

	/* select undealed orders, query the entrust result if we don't get entrust-response in 60 seconds */
	char sql[2048];
	sprintf(sql, "select * "
			" from entrust where server_name='%s' and (order_status=%d or order_status=%d)"
			" and abs(TIME_TO_SEC(TIMEDIFF(record_time, CURTIME()))) > 60 ",
			server_name_.c_str(), ORDER_STATUS_ENTRUSTING, ORDER_STATUS_WITHDRAWING);
	if (db_conn == NULL || !db_conn->_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
				"select incomplete orderid from entrust failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
	}

	while (db_conn->_conn->fetch_row())
	{
		std::string localno = db_conn->_conn->get_string("localno");
		std::string account = db_conn->_conn->get_string("account");
		std::string broker = db_conn->_conn->get_string("broker");
		if (account.empty() || localno.empty() || broker.empty()) {
			TRACE_WARNING(AT_TRACE_TAG, "localno or account of the order is not found");
			continue;
		}

		/* set timeout */
		std::string key = account + "_" + localno;
		if (map_entrust_timeout_.find(key) != map_entrust_timeout_.end()) {
			if (map_entrust_timeout_[key] > base::util::clock()) {
				continue;
			}
		}
		map_entrust_timeout_[key] = base::util::clock() + wait_timeout_;

		/* sent entrust check request */
		base::dictionary* dict = new base::dictionary();
		ref_dictionary* rd = new ref_dictionary(dict);

		(*dict)["cmd"] = ATPM_CMD_QRY_CHECKER_ENTRUST;
		(*dict)["queryid"] = get_check_id();
		(*dict)["userid"] = db_conn->_conn->get_string("userid");
		(*dict)["account"] = account;
		(*dict)["localno"] = localno;
		(*dict)["broker"] = broker;
		(*dict)["orderid"] = db_conn->_conn->get_string("orderid");
		(*dict)["entrust_type"] = db_conn->_conn->get_string("entrust_type");
		(*dict)["direction"] = db_conn->_conn->get_string("direction");
		(*dict)["systemno"] = db_conn->_conn->get_string("systemno");
		(*dict)["order_type"] = db_conn->_conn->get_long("order_type");
		(*dict)["excode"] = db_conn->_conn->get_string("excode");
		(*dict)["code"] = db_conn->_conn->get_string("code");
		(*dict)["code_name"] = db_conn->_conn->get_string("code_name");

		TRACE_DEBUG(AT_TRACE_TAG, "dict: %s", dict->to_string().c_str());

		atp_message msg;
		msg.type = ATP_MESSAGE_TYPE_CHECKER_REQ;
		msg.param1 = (void*)rd;

		mdpt_->dispatch_message(msg);

//		base::util::sleep(15);
	}
	LABEL_SCOPE_END;

end:
    if (db_conn != NULL) {
        tserver_->get_conn_pool()->retconn(db_conn);
    }
	return ret;
}

int order_checker::process_rsp_checker_entrust(base::dictionary& dict)
{
	if (dict["error_code"].int_value() != 0) {
		return NAUT_AT_S_OK;
	}
	int count = dict["count"].int_value();
	for (int i = 0; i < count; i++) {
		base::darray* da = dict["result"].array_value();
		process_rsp_checker_single_entrust(*(da->at(i).dictionary_value()));
	}
	return NAUT_AT_S_OK;
}

int order_checker::process_rsp_checker_single_entrust(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;

    naut::DBCONNECT* db_conn = NULL;

    LABEL_SCOPE_START;
    db_conn = tserver_->get_conn_pool()->getconn();

	char sql[2048];
	sprintf(sql, "select orderid, entrust_amount, entrustno, systemno"
			" from entrust where account='%s' and localno='%s'",
			dict["account"].string_value().c_str(),
			dict["localno"].string_value().c_str());
	if (db_conn == NULL || !db_conn->_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
				"select entrust by localno failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
	}

	if (db_conn->_conn->get_count() == 0) {
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_LOCALNO_NOT_EXIST, end);
	}

	int order_status = 0;
	int entrust_status = dict["order_status"].int_value();
	std::string entrustno = db_conn->_conn->get_string("entrustno");
	std::string systemno = db_conn->_conn->get_string("systemno");

	/* entrust failed */
	if (entrust_status == 7) {
		order_status = ORDER_STATUS_FAILED;
	}
	/* entrust withdrawed */
	else if (entrust_status == 6 && dict["deal_amount"].int_value() == 0) {
		order_status = ORDER_STATUS_WITHDRAWED;
	} else {
	    order_status = entrust_status;
	}
	/* entrust all dealed */
//	else if (entrust_status == 4 && dict["deal_amount"].int_value() == entrust_amount) {
//		order_status = ORDER_STATUS_DEAL_ALL;
//	}

	if (!dict["entrustno"].string_value().empty()) {
		entrustno = dict["entrustno"].string_value();
	}
	if (!dict["systemno"].string_value().empty()) {
		systemno = dict["systemno"].string_value();
	}

	if (order_status != 0) {
		sprintf(sql, "update entrust set entrustno='%s', systemno='%s',"
				" order_status='%d' where account='%s' and localno='%s'",
				entrustno.c_str(), systemno.c_str(), order_status,
				dict["account"].string_value().c_str(),
				dict["localno"].string_value().c_str());
	}
	else {
		sprintf(sql, "update entrust set entrustno='%s', systemno='%s'"
				" where account='%s' and localno='%s'",
				entrustno.c_str(), systemno.c_str(),
				dict["account"].string_value().c_str(),
				dict["localno"].string_value().c_str());
	}
	if (!db_conn->_conn->execute(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
				"update entrust order status failed, sql: '%s'", sql);
	}

	LABEL_SCOPE_END;

end:
    if (db_conn != NULL) {
        tserver_->get_conn_pool()->retconn(db_conn);
    }
	return ret;
}

void order_checker::release_messages()
{
	atp_message msg;
	while (get(msg) == 0) {
		ref_dictionary* rd = (ref_dictionary*)msg.param1;
		if (rd != NULL) {
			rd->release();
		}
	}
}

std::string order_checker::replace_quote(std::string& text)
{
	for (int i = 0; i < (int)text.length(); i++) {
		if (text[i] == '\"' || text[i] == '\'') {
			text[i] = ' ';
		}
	}
	return text;
}

}

