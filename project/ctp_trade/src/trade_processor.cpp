/*****************************************************************************
Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

FileName: trade_processor.cpp
Version: 1.0
Date: 2017.08.08

History:
eric     2017.08.08   1.0     Create
******************************************************************************/

#include "trade_processor.h"
#include "common.h"
#include "trade_server.h"
#include "base/trace.h"
#include "base/util.h"

namespace ctp
{

trade_processor::trade_processor()
	: processor_base()
    , server_name_("")
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

void trade_processor::run()
{
	atp_message msg;
	while (is_running_) {
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
	while (is_running_) {
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

int trade_processor::process_withdraw(base::dictionary& dict)
{
    int ret = NAUT_AT_S_OK;

    LABEL_SCOPE_START;
    database::dbscope mysql_db_keepper(
        tserver_->get_conn_pool());
    database::db_instance* db_conn = mysql_db_keepper.get_db_conn();
    if (db_conn == NULL) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED,
            "connect to db failed");
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CONNECT_CONFIG_DATABASE_FAILED, end);
    }
    /* check whether the order exists */
    char sql[2048];
    sprintf(sql, "select orderid, order_status, localno, entrustno, systemno, excode, code, direction,"
        " entrust_type, entrust_amount from entrust where orderid='%s'",
        dict["orderid"].string_value().c_str());
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
    /* send order response if the withdraw is failed */
    if (BFAILED(ret)) {
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
    return ret;
}


int trade_processor::process_entrust(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;

	return ret;
}

int trade_processor::process_entrust_response(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;

	return ret;
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

	return ret;
}


int trade_processor::process_withdraw_response(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;

	return ret;
}

int trade_processor::process_systemno_response(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;

	return ret;
}

int trade_processor::process_cmd_error(base::dictionary& dict)
{
    int ret = NAUT_AT_S_OK;

	return ret;
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


