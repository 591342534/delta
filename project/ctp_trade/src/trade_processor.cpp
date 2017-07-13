/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: tarde_processor.cpp
 Version: 1.0
 Date: 2016.03.21

 History:
 david wang     2016.03.21   1.0     Create
 ******************************************************************************/

#include "trade_processor.h"
#include "common.h"
#include "trade_server.h"
#include "base/trace.h"
#include "base/util.h"
#include "progress_recorder.h"

namespace ctp
{

trade_processor::trade_processor()
	: processor_base()
	, mdpt_(NULL)
	, msg_event_(NULL)
	, withdraw_queue_(NULL)
	, started_(false)
{

}

trade_processor::~trade_processor()
{
	stop();
}

int trade_processor::start(const char* server_name,
		message_dispatcher* mdpt, trade_server* ts)
{
	assert(mdpt != NULL);
	assert(ts != NULL);

	if (started_) {
		return NAUT_AT_S_OK;
	}

	server_name_ = server_name;
	mdpt_ = mdpt;
	tserver_ = ts;

	int ret = start_internal();
	if (BSUCCEEDED(ret)) {
		started_ = true;
	}
	else {
		stop();
	}
	return ret;
}

void trade_processor::stop()
{
	started_ = false;
	stop_internal();
}

void trade_processor::post(const atp_message& msg)
{
	ref_dictionary* rd = (ref_dictionary*)msg.param1;
	assert(rd != NULL);

	std::string cmd = (*(rd->get()))["cmd"].string_value();
	if (cmd == ATPM_CMD_WITHDRAW) {
		assert(withdraw_queue_ != NULL);
		withdraw_queue_->push(msg);
	}
	else {
		msg_queue_->push(msg);
	}
	if (started_) {
		assert(msg_event_ != NULL);
		msg_event_->set();
	}
}

int trade_processor::start_internal()
{
	int ret = NAUT_AT_S_OK;

	msg_event_ = new base::event();

	withdraw_queue_ = new base::srt_queue<atp_message>(8);
	withdraw_queue_->init();

	/* start process thread */
	processor_base::start();

end:
	return ret;
}

int trade_processor::stop_internal()
{
	/* stop process thread */
	processor_base::stop();

	/* release trade messages left in the queue */
	release_messages();

	if (msg_event_ != NULL) {
		delete msg_event_;
		msg_event_ = NULL;
	}

	if (withdraw_queue_ != NULL) {
		delete withdraw_queue_;
		withdraw_queue_ = NULL;
	}

	return NAUT_AT_S_OK;
}

void trade_processor::start()
{
	processor_base::start();
}

void trade_processor::run()
{
	atp_message msg;
	while (is_running_)
	{
        if(!tserver_->get_server_start_flag()) {
            base::util::sleep(1000);
            continue;
        }

		process_withdraws();

		int ret = get(msg);
		if (ret != 0) {
			msg_event_->reset();
			msg_event_->wait(20);
			continue;
		}

		ref_dictionary* rd = (ref_dictionary*)msg.param1;
		base::dictionary* dict = rd->get();
		std::string cmd = (*dict)["cmd"].string_value();

		TRACE_SYSTEM(AT_TRACE_TAG, "trade processor process packages: %s",
				dict->to_string().c_str());

		if (cmd == ATPM_CMD_ENTRUST) {
			process_entrust(*dict);
		}
		else if (cmd == ATPM_CMD_ENTRUST_RESPONSE) {
			process_entrust_response(*dict);
		}
		else if (cmd == ATPM_CMD_ENTRUST_CAPITAL_RESPONSE) {
			process_entrust_capital_response(*dict);
		}
		else if (cmd == ATPM_CMD_ORDER_STATUS_RESPONSE) {
			process_order_status_response(*dict);
		}
		else if (cmd == ATPM_CMD_DEAL) {
			process_deal(*dict);
		}
		else if (cmd == ATPM_CMD_WITHDRAW_RESPONSE) {
			process_withdraw_response(*dict);
		}
		else if (cmd == ATPM_CMD_SYSTEMNO_RESPONSE) {
			process_systemno_response(*dict);
		}
		else if (cmd == ATPM_CMD_ERROR) {
			process_cmd_error(*dict);
		}

		rd->release();
	}
}

void trade_processor::process_withdraws()
{
	atp_message msg;
	while (is_running_)
	{
		if (!withdraw_queue_->pop(msg)) {
			break;
		}

		ref_dictionary* rd = (ref_dictionary*)msg.param1;
		base::dictionary* dict = rd->get();

		std::string cmd = (*dict)["cmd"].string_value();
		assert(cmd == ATPM_CMD_WITHDRAW);

		process_withdraw(*dict);
		rd->release();
	}
}

int trade_processor::process_entrust(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;

	progress_recorder::shared_instance().record_progress(
			dict["broker"].string_value().c_str(),
			dict["account"].string_value().c_str(),
			dict["orderid"].string_value().c_str(),
			dict["msg_index"].integer_value(), ORDER_PRE_PROCESSING);

    naut::DBCONNECT* db_conn = NULL;

    LABEL_SCOPE_START;
    db_conn = tserver_->get_conn_pool()->getconn();
    if( db_conn == NULL) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED,
                "connect to db failed");
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED, end);
    }

	/* check whether the order has existed */
	char sql[2048];
	sprintf(sql, "select orderid from entrust where orderid='%s' and entrust_type=%d and broker = '%s' and account = '%s'",
			dict["orderid"].string_value().c_str(),
			dict["entrust_type"].int_value(),
			dict["broker"].string_value().c_str(),
			dict["account"].string_value().c_str());
	if (!db_conn->_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
				"select and check order from entrust failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
	}

	if (db_conn->_conn->get_count() != 0)
	{
		TRACE_WARNING(AT_TRACE_TAG, "order '%s' has existed in the table, order: %s",
				dict["orderid"].string_value().c_str(), dict.to_string().c_str());

		if(!db_conn->_conn->fetch_row()) {
	        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
	                "fetch_row failed, sql: '%s'", sql);
	        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
		}

		/* if it's a retry order, update order status and retry */
		if (dict["is_retry"].int_value() == 0 ||
			db_conn->_conn->get_long("order_status") != ORDER_STATUS_FAILED)
		{
		    TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ORDER_EXISTED,
		                        "orderid is exist, sql: '%s'", sql);
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ORDER_EXISTED, end);
		}
		else {
			sprintf(sql, "update entrust set order_status=%d, entrustno='', systemno='',"
					" localno='%s', report_time='', error_code=0, error_msg=''"
					" where orderid='%s' and entrust_type=%d",
					ORDER_STATUS_ENTRUSTING,
					dict["localno"].string_value().c_str(),
					dict["orderid"].string_value().c_str(),
					dict["entrust_type"].int_value());
			if (!db_conn->_conn->execute(sql)) {
				TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
						"update record status failed, sql: '%s'", sql);
				ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_FAILED, end);
			}
		}
	}
	else {
		/* add entrust record to database */
		sprintf(sql, "insert into entrust (server_name, orderid, localno, excode, broker,"
				" userid, account, direction, entrust_type, code, entrust_amount, entrust_price,"
				" order_type, order_time, order_time_t, order_valid_time, order_valid_time_t,"
				" record_time, order_status, error_code, flag) values ('%s', '%s', '%s', '%s', '%s', '%s', '%s',"
				" %d, %d, '%s', %d, %.3lf, %d, %ld, '%s', %ld, '%s', CURTIME(), %d, 0, 0) ",
				server_name_.c_str(), dict["orderid"].string_value().c_str(), dict["localno"].string_value().c_str(),
				dict["excode"].string_value().c_str(), dict["broker"].string_value().c_str(),
				dict["userid"].string_value().c_str(), dict["account"].string_value().c_str(),
				dict["direction"].int_value(), dict["entrust_type"].int_value(),
				dict["code"].string_value().c_str(), dict["entrust_amount"].int_value(),
				dict["entrust_price"].double_value(), dict["order_type"].int_value(),
				dict["order_time"].integer_value(),
				base::util::date_time_string(dict["order_time"].integer_value()).c_str(),
				dict["order_valid_time"].integer_value(),
				base::util::date_time_string(dict["order_valid_time"].integer_value()).c_str(),
				ORDER_STATUS_ENTRUSTING);
		if (!db_conn->_conn->execute(sql)) {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
					"insert record into entrust failed, sql: '%s'", sql);
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_FAILED, end);
		}
	}

	/* check whether the order is expired */
	long order_time = dict["order_time"].integer_value();
	long order_valid_time = dict["order_valid_time"].integer_value();
	long ltime = base::util::local_timestamp();
	if (order_valid_time < ltime && order_valid_time != order_time)
	{
		TRACE_WARNING(AT_TRACE_TAG, "entrust order is expired, '%s'",
				dict.to_string().c_str());

		sprintf(sql, "update entrust set order_status=%d, error_code=%d,"
				" error_msg='%s' where orderid='%s' and entrust_type=%d",
				ORDER_STATUS_FAILED, AT_ERROR_ORDER_EXPIRED,
				get_response_error_msg(AT_ERROR_ORDER_EXPIRED).c_str(),
				dict["orderid"].string_value().c_str(),
				dict["entrust_type"].int_value());
		if (!db_conn->_conn->execute(sql)) {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
					"update record status failed, sql: '%s'", sql);
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_FAILED, end);
		}
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ORDER_EXPIRED, end);
	}

	/* post message for further processing */
	base::dictionary* pdict = new base::dictionary(dict);
	ref_dictionary* rd = new ref_dictionary(pdict);

	atp_message msg;
	msg.type = ATP_MESSAGE_TYPE_SERVER_TRADE_REQ;
	msg.param1 = (void*)rd;
	mdpt_->dispatch_message(msg);

	LABEL_SCOPE_END;

end:
    if (db_conn != NULL) {
        tserver_->get_conn_pool()->retconn(db_conn);
    }
	/* send order response if the order is failed */
	if (BFAILED(ret))
	{
		base::dictionary* pdict = new base::dictionary(dict);
		ref_dictionary* rd = new ref_dictionary(pdict);

		(*pdict)["cmd"] = ATPM_CMD_ENTRUST_RESULT;
		(*pdict)["error_code"] = get_response_error_code(ret);
		(*pdict)["error_msg"] = get_response_error_msg((*pdict)["error_code"].integer_value());

		atp_message msg;
		msg.type = ATP_MESSAGE_TYPE_ORDER_REPLY;
		msg.param1 = (void*)rd;
		mdpt_->dispatch_message(msg);
	}
	/* record order progress */
	progress_recorder::shared_instance().record_progress(
			dict["broker"].string_value().c_str(),
			dict["account"].string_value().c_str(),
			dict["orderid"].string_value().c_str(),
			dict["msg_index"].integer_value(), ORDER_PRE_PREOCESSED);
	return ret;
}

int trade_processor::process_entrust_response(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;

    naut::DBCONNECT* db_conn = NULL;

    LABEL_SCOPE_START;
    db_conn = tserver_->get_conn_pool()->getconn();
    if( db_conn == NULL) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED,
                "connect to db failed");
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED, end);
    }


	/* check whether the order has existed */
	char sql[2048];
	sprintf(sql, "select orderid, entrust_amount, entrust_price,"
			" entrust_type, direction, code, excode, broker"
			" from entrust where account='%s' and localno='%s'",
			dict["account"].string_value().c_str(), dict["localno"].string_value().c_str());
	if (!db_conn->_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
				"select and check order from entrust failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
	}
	if (db_conn->_conn->get_count() == 0) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ORDER_NOT_EXIST,
				"entrust record is not exist, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ORDER_NOT_EXIST, end);
	}

	/* update entrust result */
	int at_error = dict["at_error"].int_value();

	if (at_error == AT_ERROR_NONE) {
		sprintf(sql, "update entrust set systemno='%s', entrustno='%s',"
				" entrust_date='%s', entrust_time='%s', error_code=0, flag=%d,"
				" order_status=%d where account='%s' and localno='%s'",
				dict["systemno"].string_value().c_str(),
				dict["entrustno"].string_value().c_str(),
				dict["entrust_date"].string_value().c_str(),
				dict["entrust_time"].string_value().c_str(),
				dict["is_risk_order"].int_value(),
				ORDER_STATUS_ENTRUSTED,
				dict["account"].string_value().c_str(),
				dict["localno"].string_value().c_str());
	}
	else {
	    string error_msg = get_response_error_msg(at_error);
	    if(at_error >= AT_ERROR_TRADER) {
	        at_error = AT_ERROR_TRADER;
	    }
		sprintf(sql, "update entrust set systemno='%s', entrustno='%s',"
				" entrust_date='%s', entrust_time='%s', error_code=%d, error_msg='%s',"
				" order_status=%d where account='%s' and localno='%s'",
				dict["systemno"].string_value().c_str(),
				dict["entrustno"].string_value().c_str(),
				dict["entrust_date"].string_value().c_str(),
				dict["entrust_time"].string_value().c_str(),
				at_error,
				error_msg.c_str(),
				ORDER_STATUS_FAILED,
				dict["account"].string_value().c_str(),
				dict["localno"].string_value().c_str());

		/* the order is failed, send response */
		base::dictionary* pdict = new base::dictionary(dict);
		ref_dictionary* rd = new ref_dictionary(pdict);

		if(!db_conn->_conn->fetch_row()) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
                    "fetch_row failed, sql: '%s'", sql);
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
		}

		(*pdict)["cmd"] = ATPM_CMD_ENTRUST_RESULT;
		(*pdict)["error_code"] = at_error;
		(*pdict)["error_msg"] = error_msg;
		(*pdict)["orderid"] = db_conn->_conn->get_string("orderid");
		(*pdict)["entrust_amount"] = db_conn->_conn->get_long("entrust_amount");
		(*pdict)["entrust_price"] = db_conn->_conn->get_string("entrust_price");
		(*pdict)["entrust_type"] = db_conn->_conn->get_long("entrust_type");
		(*pdict)["direction"] = db_conn->_conn->get_long("direction");
		(*pdict)["code"] = db_conn->_conn->get_string("code");
		(*pdict)["excode"] = db_conn->_conn->get_string("excode");
		(*pdict)["broker"] = db_conn->_conn->get_string("broker");

		atp_message msg;
		msg.type = ATP_MESSAGE_TYPE_ORDER_REPLY;
		msg.param1 = (void*)rd;
		mdpt_->dispatch_message(msg);
	}

	if (!db_conn->_conn->execute(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
				"update entrust result failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_FAILED, end);
	}

	LABEL_SCOPE_END;

end:
    if (db_conn != NULL) {
        tserver_->get_conn_pool()->retconn(db_conn);
    }
	return ret;
}

int trade_processor::process_entrust_capital_response(base::dictionary& dict)
{
	return NAUT_AT_S_OK;
}

int trade_processor::process_order_status_response(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;

	LABEL_SCOPE_START;

	std::vector<atp_message>::iterator it = withdraw_wait_queue_.begin();
	for (; it != withdraw_wait_queue_.end(); ) {
		atp_message& msg = (*it);
		ref_dictionary* wait_rd = (ref_dictionary*)msg.param1;
		base::dictionary* wait_dict = wait_rd->get();
		if ((*wait_dict)["entrustno"].string_value() == dict["entrustno"].string_value() &&
			(*wait_dict)["account"].string_value() == dict["account"].string_value())
		{
			base::dictionary* pdict = new base::dictionary(*wait_dict);
			ref_dictionary* rd = new ref_dictionary(pdict);

			(*pdict)["cmd"] = ATPM_CMD_WITHDRAW_RESULT;

			if (dict["canceled"].string_value() == "0") {
				(*pdict)["error_code"] = AT_ERROR_WITHDRAW_FAILED;
				(*pdict)["error_msg"] = get_response_error_msg(AT_ERROR_WITHDRAW_FAILED, NULL);
			}
			else {
				(*pdict)["error_code"] = (int)0;
			}
			atp_message msg;
			msg.type = ATP_MESSAGE_TYPE_ORDER_REPLY;
			msg.param1 = (void*)rd;
			mdpt_->dispatch_message(msg);

			it = withdraw_wait_queue_.erase(it);
		}
		else {
			++it;
		}
	}

	LABEL_SCOPE_END;
end:
	return ret;
}

int trade_processor::process_deal(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;

    naut::DBCONNECT* db_conn = NULL;

    LABEL_SCOPE_START;
    db_conn = tserver_->get_conn_pool()->getconn();
    if(db_conn == NULL) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED,
                "connect to db failed");
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED, end);
    }

	/* check where the deal has existed */
	char sql[2048];
	sprintf(sql, "select dealno from deal where dealno='%s' and deal_date='%s' and account='%s' "
	        "and broker = '%s' and entrustno = '%s' and entrust_type = %ld",
			dict["dealno"].string_value().c_str(),
			dict["deal_date"].string_value().c_str(),
			dict["account"].string_value().c_str(),
			dict["broker"].string_value().c_str(),
			dict["entrustno"].string_value().c_str(),
			dict["entrust_type"].integer_value());
	if (!db_conn->_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
				"select and check dealno from deal failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
	}
	if (db_conn->_conn->get_count() > 0) {
		TRACE_WARNING(AT_TRACE_TAG, "dealno has already existed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_DEAL_EXISTED, end);
	}

	//{"queryid":"order_checker_4","account":"123321123322","systemno":"103022434","broker":"����","entrust_type":"1",
	// "direction":"1","orderid":"St160331000724_0","cmd":"qry_checker_deal","userid":"123321123322"}

	/* check whether the order exists */
	if(!dict["localno"].string_value().empty()) {
	    sprintf(sql, "select * from entrust where account='%s' and localno='%s' and broker = '%s' and entrust_type = %ld ",
	            dict["account"].string_value().c_str(),
	            dict["localno"].string_value().c_str(),
	            dict["broker"].string_value().c_str(),
	            dict["entrust_type"].integer_value());
	} else if(!dict["systemno"].string_value().empty()) {
        sprintf(sql, "select * from entrust where account='%s' and systemno='%s' and broker = '%s' and entrust_type = %ld ",
                dict["account"].string_value().c_str(),
                dict["systemno"].string_value().c_str(),
                dict["broker"].string_value().c_str(),
                dict["entrust_type"].integer_value());
    } else {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
                "systemno and localno both empty: '%s'", sql);
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
    }

	if (!db_conn->_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
				"select and check order from entrust failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
	}

	if (db_conn->_conn->get_count() != 1 || !db_conn->_conn->fetch_row()) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ENTRUST_CHECK_ERROR,
				"entrust is not exist or not unique, account: %s, localno: %s, count: %d, sql:%s",
				dict["account"].string_value().c_str(), dict["localno"].string_value().c_str(),
				db_conn->_conn->get_count(), sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ENTRUST_CHECK_ERROR, end);
	}

	std::string userid = db_conn->_conn->get_string("userid");
	std::string excode = db_conn->_conn->get_string("excode");
	std::string orderid = db_conn->_conn->get_string("orderid");
	std::string entrustid = db_conn->_conn->get_string("entrustid");
	std::string systemno = db_conn->_conn->get_string("systemno");
	int entrust_amount = db_conn->_conn->get_long("entrust_amount");
	std::string entrust_price = db_conn->_conn->get_string("entrust_price");
	int entrust_type = db_conn->_conn->get_long("entrust_type");
	int direction = db_conn->_conn->get_long("direction");
	dict["order_type"] = db_conn->_conn->get_long("order_type");
	/* insert deal record to table */
	int at_error = dict["at_error"].int_value();
	std::string error_msg;
	if (at_error != AT_ERROR_NONE) {
		error_msg = get_response_error_msg(at_error);
		if(at_error >= AT_ERROR_TRADER) {
		    at_error = AT_ERROR_TRADER;
		}
	}

	sprintf(sql, "insert into deal (server_name, entrustid, entrustno, dealno, orderid, broker,"
			" userid, account, code, excode, deal_amount, deal_price, deal_fund, deal_date,"
			" deal_time, error_code, error_msg, record_time, direction, entrust_type, code_name) values ('%s', '%s', '%s', '%s',"
			" '%s', '%s', '%s', '%s', '%s', '%s', %d, %.3f, %.3lf, '%s', '%s', %d, '%s',CURTIME(), %d, %d, '%s')",
			server_name_.c_str(), entrustid.c_str(), dict["entrustno"].string_value().c_str(),
			dict["dealno"].string_value().c_str(), orderid.c_str(), dict["broker"].string_value().c_str(),
			userid.c_str(), dict["account"].string_value().c_str(),
			dict["code"].string_value().c_str(), excode.c_str(),
			dict["deal_amount"].int_value(), dict["deal_price"].double_value(),
			dict["deal_fund"].double_value(), dict["deal_date"].string_value().c_str(),
			dict["deal_time"].string_value().c_str(), at_error, error_msg.c_str(),
			direction, entrust_type, dict["code_name"].string_value().c_str());

	if (!db_conn->_conn->execute(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
				"insert record into deal failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_FAILED, end);
	}

	int order_status = ORDER_STATUS_DEAL;
	/* update entrust order status and report time */
	if (at_error == AT_ERROR_NONE) {
		if (entrust_amount != 0 && dict["deal_amount"].int_value() == entrust_amount) {
			order_status = ORDER_STATUS_DEAL_ALL;
		}
		else {
			/* if there exists multiple deal records for the entrust, check the sum */
			sprintf(sql, "select sum(deal_amount) as total_deal_amount from"
					" deal where entrustno='%s' and account='%s'  and deal_date='%s' and broker = '%s' and entrust_type = %ld ",
					dict["entrustno"].string_value().c_str(),
					dict["account"].string_value().c_str(),
					dict["deal_date"].string_value().c_str(),
					dict["broker"].string_value().c_str(),
					dict["entrust_type"].integer_value()
					);
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
				" entrustno='%s' where account='%s' and localno='%s'",
				order_status, dict["deal_time"].string_value().c_str(),
				dict["entrustno"].string_value().c_str(),
				dict["account"].string_value().c_str(),
				dict["localno"].string_value().c_str());
		if (!db_conn->_conn->execute(sql)) {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
					"update entrust order status failed, sql: '%s'", sql);
		}
		/* if the order has already completed, send response */
		if (order_status == ORDER_STATUS_DEAL_ALL)
		{
			base::dictionary* pdict = new base::dictionary(dict);
			ref_dictionary* rd = new ref_dictionary(pdict);

			(*pdict)["cmd"] = ATPM_CMD_ENTRUST_RESULT;
			(*pdict)["error_code"] = (int)0;
			(*pdict)["orderid"] = orderid;
			(*pdict)["entrustid"] = entrustid;
			(*pdict)["entrust_amount"] = entrust_amount;
			(*pdict)["entrust_price"] = entrust_price;
			(*pdict)["entrust_type"] = entrust_type;
			(*pdict)["direction"] = direction;
			(*pdict)["order_status"] = (long)order_status;

			atp_message msg;
			msg.type = ATP_MESSAGE_TYPE_ORDER_REPLY;
			msg.param1 = (void*)rd;
			mdpt_->dispatch_message(msg);
		}
	}

	/* push deal message */
	base::dictionary* pdict = new base::dictionary(dict);
	ref_dictionary* rd = new ref_dictionary(pdict);

	(*pdict)["cmd"] = ATPM_CMD_DEAL;
	(*pdict)["orderid"] = orderid;
	(*pdict)["entrustid"] = entrustid;
	(*pdict)["entrust_amount"] = entrust_amount;
	(*pdict)["entrust_price"] = entrust_price;
	(*pdict)["entrust_type"] = entrust_type;
	(*pdict)["direction"] = direction;
	(*pdict)["error_code"] = at_error;
	(*pdict)["error_msg"] = error_msg;
	(*pdict)["order_status"] = (long)order_status;

	TRACE_INFO(AT_TRACE_TAG, "pdict: %s", pdict->to_string().c_str());

	atp_message msg;
	msg.type = ATP_MESSAGE_TYPE_DEAL_PUSH;
	msg.param1 = (void*)rd;
	mdpt_->dispatch_message(msg);

	LABEL_SCOPE_END;

end:
    if (db_conn != NULL) {
        tserver_->get_conn_pool()->retconn(db_conn);
    }

	return ret;
}

int trade_processor::process_withdraw(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;

	progress_recorder::shared_instance().record_progress(
			dict["broker"].string_value().c_str(),
			dict["account"].string_value().c_str(),
			dict["orderid"].string_value().c_str(),
			dict["msg_index"].integer_value(), ORDER_PRE_PROCESSING);

    naut::DBCONNECT* db_conn = NULL;

    LABEL_SCOPE_START;
    db_conn = tserver_->get_conn_pool()->getconn();
    if(db_conn == NULL) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED,
                "connect to db failed");
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED, end);
    }
	/* check whether the order exists */
	char sql[2048];
	sprintf(sql, "select orderid, order_status, localno, entrustno, systemno, excode, code, direction,"
			" entrust_type, entrust_amount from entrust where orderid='%s' and entrust_type=%d",
			dict["orderid"].string_value().c_str(), dict["entrust_type"].int_value());
	if (!db_conn->_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
				"select and check order from entrust failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
	}

	/* if order not exist, insert record to database */
	if (db_conn->_conn->get_count() == 0) {
		sprintf(sql, "insert into entrust (server_name, orderid, localno, excode, broker,"
				" userid, account, direction, entrust_type, code, entrust_amount, entrust_price,"
				" order_type, order_time, order_time_t, order_valid_time, order_valid_time_t,"
				" record_time, order_status, flag) values ('%s', '%s', '%s', '%s', '%s', '%s', '%s',"
				" %d, %d, '%s', %d, %.3lf, %d, %ld, '%s', %ld, '%s', CURTIME(), %d, 0) ",
				server_name_.c_str(), dict["orderid"].string_value().c_str(), dict["localno"].string_value().c_str(),
				dict["excode"].string_value().c_str(), dict["broker"].string_value().c_str(),
				dict["userid"].string_value().c_str(), dict["account"].string_value().c_str(),
				dict["direction"].int_value(), dict["entrust_type"].int_value(),
				dict["code"].string_value().c_str(), dict["entrust_amount"].int_value(),
				dict["entrust_price"].double_value(), dict["order_type"].int_value(),
				dict["order_time"].integer_value(),
				base::util::date_time_string(dict["order_time"].integer_value()).c_str(),
				dict["order_valid_time"].integer_value(),
				base::util::date_time_string(dict["order_valid_time"].integer_value()).c_str(),
				ORDER_STATUS_WITHDRAWED);
		if (!db_conn->_conn->execute(sql)) {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
					"insert record into entrust failed, sql: '%s'", sql);
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_FAILED, end);
		}

		base::dictionary* pdict = new base::dictionary(dict);
		ref_dictionary* rd = new ref_dictionary(pdict);

		(*pdict)["cmd"] = ATPM_CMD_WITHDRAW_RESULT;
		(*pdict)["error_code"] = (int)0;

		atp_message msg;
		msg.type = ATP_MESSAGE_TYPE_ORDER_REPLY;
		msg.param1 = (void*)rd;
		mdpt_->dispatch_message(msg);
	}
	else {
		db_conn->_conn->fetch_row();

		std::string systemno = db_conn->_conn->get_string("systemno");
		if (systemno.length() == 0) {
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_SYSTEMNO_NOT_EXIST, end);
		}

		int order_status = db_conn->_conn->get_long("order_status");
		if (order_status == ORDER_STATUS_DEAL_ALL) {
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ORDER_HAS_BEEN_DEALED, end);
		}
		else if (order_status == ORDER_STATUS_WITHDRAWED) {
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ORDER_HAS_BEEN_WITHDRAWED, end);
		}
		else if (order_status == ORDER_STATUS_FAILED) {
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ORDER_IS_FAILED, end);
		}

		/* check whether the order has dealed */
		std::string entrustno = db_conn->_conn->get_string("entrustno");
		std::string localno = db_conn->_conn->get_string("localno");
		int entrust_amount = db_conn->_conn->get_long("entrust_amount");
		int direction = db_conn->_conn->get_long("direction");
		int entrust_type = db_conn->_conn->get_long("entrust_type");

		if (entrustno.empty()) {
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ENTRUSTNO_NOT_EXIST, end);
		}

		/* post message to trade unit for further processing */
		base::dictionary* pdict = new base::dictionary(dict);
		ref_dictionary* rd = new ref_dictionary(pdict);

		(*pdict)["systemno"] = systemno;
		(*pdict)["entrustno"] = entrustno;
		(*pdict)["direction"] = direction;
		(*pdict)["entrust_type"] = entrust_type;
		(*pdict)["entrust_amount"] = entrust_amount;
		(*pdict)["localno"] = localno;

		atp_message msg;
		msg.type = ATP_MESSAGE_TYPE_SERVER_TRADE_REQ;
		msg.param1 = (void*)rd;

		rd->retain();
		withdraw_wait_queue_.push_back(msg);

		mdpt_->dispatch_message(msg);
	}

	LABEL_SCOPE_END;

end:
    if (db_conn != NULL) {
        tserver_->get_conn_pool()->retconn(db_conn);
    }
	/* send order response if the withdraw is failed */
	if (BFAILED(ret))
	{
		base::dictionary* pdict = new base::dictionary(dict);
		ref_dictionary* rd = new ref_dictionary(pdict);

		(*pdict)["cmd"] = ATPM_CMD_WITHDRAW_RESULT;
		(*pdict)["error_code"] = get_response_error_code(ret);
		(*pdict)["error_msg"] = get_response_error_msg((*pdict)["error_code"].integer_value());

		atp_message msg;
		msg.type = ATP_MESSAGE_TYPE_ORDER_REPLY;
		msg.param1 = (void*)rd;
		mdpt_->dispatch_message(msg);
	}
	/* record order progress */
	progress_recorder::shared_instance().record_progress(
			dict["broker"].string_value().c_str(),
			dict["account"].string_value().c_str(),
			dict["orderid"].string_value().c_str(),
			dict["msg_index"].integer_value(), ORDER_PRE_PREOCESSED);
	return ret;
}

int trade_processor::process_withdraw_response(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;

    naut::DBCONNECT* db_conn = NULL;

    LABEL_SCOPE_START;
    db_conn = tserver_->get_conn_pool()->getconn();
    if(db_conn == NULL) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED,
                "connect to db failed");
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED, end);
    }
	/* check whether the order has existed */
	char sql[2048];
	sprintf(sql, "select orderid, order_status from entrust where account='%s' and systemno='%s'",
			dict["account"].string_value().c_str(), dict["systemno"].string_value().c_str());
	if (!db_conn->_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
				"select and check order from entrust failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
	}
	if (db_conn->_conn->get_count() == 0) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ORDER_NOT_EXIST,
				"entrust record is not exist, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ORDER_NOT_EXIST, end);
	}

	/* update entrust result */
	int at_error = dict["at_error"].int_value();
	if (at_error == AT_ERROR_NONE)
	{
		if (dict["entrust_amount"].int_value() == dict["withdraw_amount"].int_value() &&
			dict["deal_amount"].int_value() == 0)
		{
			sprintf(sql, "update entrust set order_status=%d, report_time='%s'"
					" where account='%s' and systemno='%s'",
					ORDER_STATUS_WITHDRAWED,
					dict["withdraw_time"].string_value().c_str(),
					dict["account"].string_value().c_str(),
					dict["systemno"].string_value().c_str());
			if (!db_conn->_conn->execute(sql)) {
				TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
						"update entrust-withdraw status failed, sql: '%s'", sql);
				ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_FAILED, end);
			}
		}
	}

	LABEL_SCOPE_END;

end:
    if (db_conn != NULL) {
        tserver_->get_conn_pool()->retconn(db_conn);
    }
	std::vector<atp_message>::iterator it = withdraw_wait_queue_.begin();
	for (; it != withdraw_wait_queue_.end(); ) {
		atp_message& msg = (*it);
		ref_dictionary* wait_rd = (ref_dictionary*)msg.param1;
		base::dictionary* wait_dict = wait_rd->get();
		if ((*wait_dict)["entrustno"].string_value() == dict["entrustno"].string_value() &&
			(*wait_dict)["account"].string_value() == dict["account"].string_value())
		{
			base::dictionary* pdict = new base::dictionary(dict);
			ref_dictionary* rd = new ref_dictionary(pdict);

			(*pdict)["cmd"] = ATPM_CMD_WITHDRAW_RESULT;
			(*pdict)["orderid"] = (*wait_dict)["orderid"].string_value();
			(*pdict)["userid"] = (*wait_dict)["userid"].string_value();
			(*pdict)["account"] = (*wait_dict)["account"].string_value();
			(*pdict)["code"] = (*wait_dict)["code"].string_value();
			int at_error = dict["at_error"].int_value();
			string error_msg = get_response_error_msg(at_error);
			at_error = at_error >= AT_ERROR_TRADER ? AT_ERROR_TRADER : at_error;
			(*pdict)["error_code"] = (long)at_error;
			(*pdict)["error_msg"] = error_msg;

			atp_message msg;
			msg.type = ATP_MESSAGE_TYPE_ORDER_REPLY;
			msg.param1 = (void*)rd;
			mdpt_->dispatch_message(msg);

			it = withdraw_wait_queue_.erase(it);
		}
		else {
			++it;
		}
	}
	return ret;
}

int trade_processor::process_systemno_response(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;

    naut::DBCONNECT* db_conn = NULL;

    LABEL_SCOPE_START;
    db_conn = tserver_->get_conn_pool()->getconn();
    if(db_conn == NULL) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED,
                "connect to db failed");
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED, end);
    }
	/* check whether the order has existed */
	char sql[2048];
	sprintf(sql, "select orderid from entrust where account='%s' and localno='%s'",
			dict["account"].string_value().c_str(),
			dict["localno"].string_value().c_str());
	if (!db_conn->_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED,
				"select and check order from entrust failed, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_QUERY_FAILED, end);
	}
	if (db_conn->_conn->get_count() == 0) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ORDER_NOT_EXIST,
				"entrust record is not exist, sql: '%s'", sql);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ORDER_NOT_EXIST, end);
	}

	/* update entrust result */
	int at_error = dict["at_error"].int_value();
	if (at_error == AT_ERROR_NONE) {
		sprintf(sql, "update entrust set systemno='%s' where account='%s' and localno='%s'",
				dict["requestid"].string_value().c_str(),
				dict["account"].string_value().c_str(),
				dict["localno"].string_value().c_str());
		if (!db_conn->_conn->execute(sql)) {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
					"update entrust systemno failed, sql: '%s'", sql);
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_EXCUTE_DB_FAILED, end);
		}
	}

	LABEL_SCOPE_END;

end:
    if (db_conn != NULL) {
        tserver_->get_conn_pool()->retconn(db_conn);
    }
	return ret;
}

int trade_processor::process_cmd_error(base::dictionary& dict)
{
    int ret = NAUT_AT_S_OK;
	assert(dict["error_code"].int_value() != 0);
    naut::DBCONNECT* db_conn = NULL;

    LABEL_SCOPE_START;
    db_conn = tserver_->get_conn_pool()->getconn();
    if(db_conn == NULL) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED,
                "connect to db failed");
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED, end);
    }
	bool need_reply = true;
	std::string cmd = dict["real_cmd"].string_value();
	if (cmd == ATPM_CMD_ENTRUST)
	{
		cmd = ATPM_CMD_ENTRUST_RESULT;

		char sql[4096];
		sprintf(sql, "update entrust set order_status=%d, error_code=%d,"
				" error_msg='%s' where orderid='%s' and entrust_type=%d",
				ORDER_STATUS_FAILED, dict["error_code"].int_value(),
				dict["error_msg"].string_value().c_str(),
				dict["orderid"].string_value().c_str(),
				dict["entrust_type"].int_value());
		if (!db_conn->_conn->execute(sql)) {
			TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
					"update record status failed, sql: '%s'", sql);
		}
	}
	else if (cmd == ATPM_CMD_WITHDRAW) {
		cmd = ATPM_CMD_WITHDRAW_RESULT;
	}
	else {
		need_reply = false;
	}

	if (need_reply)
	{
		base::dictionary* pdict = new base::dictionary(dict);
		ref_dictionary* rd = new ref_dictionary(pdict);

		(*pdict)["cmd"] = cmd;
		pdict->remove_object("real_cmd");

		atp_message msg;
		msg.type = ATP_MESSAGE_TYPE_ORDER_REPLY;
		msg.param1 = (void*)rd;
		mdpt_->dispatch_message(msg);
	}
	LABEL_SCOPE_END;
end:
    if (db_conn != NULL) {
        tserver_->get_conn_pool()->retconn(db_conn);
    }
	return ret;
}

void trade_processor::release_messages()
{
	atp_message msg;
	while (get(msg) == 0) {
		ref_dictionary* rd = (ref_dictionary*)msg.param1;
		if (rd != NULL) {
			rd->release();
		}
	}
	if (withdraw_queue_ != NULL) {
		while (withdraw_queue_->pop(msg)) {
			ref_dictionary* rd = (ref_dictionary*)msg.param1;
			if (rd != NULL) {
				rd->release();
			}
		}
	}
	for (int i = 0; i < (int)withdraw_wait_queue_.size(); i++) {
		ref_dictionary* rd = (ref_dictionary*)withdraw_wait_queue_[i].param1;
		if (rd != NULL) {
			rd->release();
		}
	}
	withdraw_wait_queue_.clear();
}

std::string trade_processor::replace_quote(std::string& text)
{
	for (int i = 0; i < (int)text.length(); i++) {
		if (text[i] == '\"' || text[i] == '\'') {
			text[i] = ' ';
		}
	}
	return text;
}

}


