/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: query_processor.cpp
 Version: 1.0
 Date: 2017.08.08

 History:
 eric     2017.08.08   1.0     Create
 ******************************************************************************/

#include "query_processor.h"
#include "common.h"
#include "trade_server.h"
#include "base/trace.h"
#include "base/util.h"

namespace ctp
{

query_processor::query_processor()
	: processor_base()
	, mdpt_(NULL)
	, msg_event_(NULL)
	, started_(false)
{

}

query_processor::~query_processor()
{
	stop();
}

int query_processor::start(const char* server_name,
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

void query_processor::post(const atp_message& msg)
{
	msg_queue_->push(msg);

	if (started_) {
		assert(msg_event_ != NULL);
		msg_event_->set();
	}
}

void query_processor::stop()
{
	started_ = false;
	stop_internal();
}

void query_processor::start()
{
	processor_base::start();
}

int query_processor::start_internal()
{
	int ret = NAUT_AT_S_OK;

	msg_event_ = new base::event();

	/* start process thread */
	processor_base::start();

end:
	return ret;
}

int query_processor::stop_internal()
{
	/* stop process thread */
	processor_base::stop();

	/* release trade messages left in the queue */
	release_messages();

	if (msg_event_ != NULL) {
		delete msg_event_;
		msg_event_ = NULL;
	}

	VBASE_HASH_MAP<std::string, base::dictionary*>::iterator mit = map_dict_account_.begin();
	for (; mit != map_dict_account_.end(); mit++) {
		delete mit->second;
		mit->second = NULL;
	}
	map_dict_account_.clear();

	mit = map_dict_entrust_.begin();
	for (; mit != map_dict_entrust_.end(); mit++) {
		delete mit->second;
		mit->second = NULL;
	}
	map_dict_entrust_.clear();

	mit = map_dict_openinter_.begin();
	for (; mit != map_dict_openinter_.end(); mit++) {
		delete mit->second;
		mit->second = NULL;
	}
	map_dict_openinter_.clear();

	mit = map_dict_deal_.begin();
	for (; mit != map_dict_deal_.end(); mit++) {
		delete mit->second;
		mit->second = NULL;
	}
	map_dict_deal_.clear();

	return NAUT_AT_S_OK;
}

void query_processor::run()
{
	atp_message msg;
	while (is_running_)
	{
        if(!tserver_->get_server_start_flag()) {
            base::util::sleep(1000);
            continue;
        }

		check_query_wait_timeout();

		int ret = get(msg);
		if (ret != 0) {
			msg_event_->reset();
			msg_event_->wait(50);
			continue;
		}

		ref_dictionary* rd = (ref_dictionary*)msg.param1;
		base::dictionary* dict = rd->get();
		std::string cmd = (*dict)["cmd"].string_value();

		TRACE_SYSTEM(AT_TRACE_TAG, "query processor process packages: %s",
				dict->to_string().c_str());

		if (cmd == ATPM_CMD_QRY_ACCOUNT) {
			process_qry_account(*dict);
		}
		else if (cmd == ATPM_CMD_QRY_OPENINTER) {
			process_qry_openinter(*dict);
		}
		else if (cmd == ATPM_CMD_QRY_ENTRUST) {
			process_qry_entrust(*dict);
		}
		else if (cmd == ATPM_CMD_QRY_CHECKER_ENTRUST) {
			process_qry_checker_entrust(*dict);
		}
		else if (cmd == ATPM_CMD_QRY_DEAL) {
			process_qry_deal(*dict);
		}
		else if (cmd == ATPM_CMD_QRY_CHECKER_DEAL) {
			process_qry_checker_deal(*dict);
		}
		else if (cmd == ATPM_CMD_RSP_ACCOUNT) {
			process_rsp_account(*dict);
		}
		else if (cmd == ATPM_CMD_RSP_OPENINTER) {
			process_rsp_openinter(*dict);
		}
		else if (cmd == ATPM_CMD_RSP_ENTRUST) {
			process_rsp_entrust(*dict);
		}
		else if (cmd == ATPM_CMD_RSP_DEAL) {
			process_rsp_deal(*dict);
		}
		else if (cmd == ATPM_CMD_RSP_CHECKER_DEAL) {
			process_rsp_checker_deal(*dict);
		}
		else if (cmd == ATPM_CMD_ERROR) {
			process_cmd_error(*dict);
		}

		rd->release();
	}
}

int query_processor::process_qry_account(base::dictionary& dict)
{
	base::dictionary* pdict = new base::dictionary(dict);
	ref_dictionary* rd = new ref_dictionary(pdict);

	atp_message msg;
	msg.type = ATP_MESSAGE_TYPE_SERVER_QUERY_REQ;
	msg.param1 = (void*)rd;

	rd->retain();
	msg.param2 = (void*)(base::util::clock() + QUERY_WAIT_TIMEOUT);
	wait_match_queue_.push_back(msg);

	mdpt_->dispatch_message(msg);

	return NAUT_AT_S_OK;
}

int query_processor::process_qry_openinter(base::dictionary& dict)
{
	base::dictionary* pdict = new base::dictionary(dict);
	ref_dictionary* rd = new ref_dictionary(pdict);

	atp_message msg;
	msg.type = ATP_MESSAGE_TYPE_SERVER_QUERY_REQ;
	msg.param1 = (void*)rd;

	rd->retain();
	msg.param2 = (void*)(base::util::clock() + QUERY_WAIT_TIMEOUT);
	wait_match_queue_.push_back(msg);

	mdpt_->dispatch_message(msg);

	return NAUT_AT_S_OK;
}

int query_processor::process_qry_entrust(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;

	return ret;
}

int query_processor::process_qry_checker_entrust(base::dictionary& dict)
{
	assert(!dict["localno"].string_value().empty());
	assert(!dict["account"].string_value().empty());

	/* send query request to trade server */
	base::dictionary* pdict = new base::dictionary(dict);
	ref_dictionary* rd = new ref_dictionary(pdict);

	atp_message msg;
	msg.type = ATP_MESSAGE_TYPE_SERVER_QUERY_REQ;
	msg.param1 = (void*)rd;

	rd->retain();
	msg.param2 = (void*)(base::util::clock() + QUERY_WAIT_TIMEOUT);
	wait_match_queue_.push_back(msg);

	mdpt_->dispatch_message(msg);

	return NAUT_AT_S_OK;
}

int query_processor::process_qry_deal(base::dictionary& dict)
{
	int ret = NAUT_AT_S_OK;

	return ret;
}

int query_processor::process_qry_checker_deal(base::dictionary& dict)
{
	std::string systemno = dict["systemno"].string_value();
	assert(!systemno.empty());

	base::dictionary* pdict = new base::dictionary(dict);
	ref_dictionary* rd = new ref_dictionary(pdict);

	(*pdict)["systemno"] = systemno;

	atp_message msg;
	msg.type = ATP_MESSAGE_TYPE_SERVER_QUERY_REQ;
	msg.param1 = (void*)rd;

	rd->retain();
	msg.param2 = (void*)(base::util::clock() + QUERY_WAIT_TIMEOUT);
	wait_match_queue_.push_back(msg);

	mdpt_->dispatch_message(msg);

	return NAUT_AT_S_OK;
}

int query_processor::process_rsp_account(base::dictionary& dict)
{
	std::string account_key = trade_server::get_account_broker_bs_key(dict["broker"].string_value(), dict["account"].string_value());
	base::dictionary* account_dict = map_dict_account_[account_key];
	if (account_dict == NULL) {
		account_dict = new base::dictionary();
		(*account_dict)["count"] = (int)0;
		(*account_dict)["result"] = new base::darray();
		(*account_dict)["account"] = dict["account"].string_value();
		(*account_dict)["userid"] = dict["userid"].string_value();
		(*account_dict)["broker"] = dict["broker"].string_value();
		map_dict_account_[account_key] = account_dict;
	}

	/* add result record */
	(*account_dict)["count"] = (*account_dict)["count"].int_value() + 1;
	(*account_dict)["result"].array_value()->append(dict);
	if (dict["result_complete"].string_value() != "1") {
		(*account_dict)["complete"] = 0;
	}
	else {
		(*account_dict)["complete"] = 1;
	}

	/* check whether the result is completed */
	if (dict["result_complete"].string_value() == "1") {
		std::vector<atp_message>::iterator it = wait_match_queue_.begin();
		for (; it != wait_match_queue_.end(); ) {
			atp_message& qmsg = (*it);
			ref_dictionary* qrd = (ref_dictionary*)qmsg.param1;
			base::dictionary& matcher_dict = *(qrd->get());
			if (matcher_dict["cmd"].string_value() == ATPM_CMD_QRY_ACCOUNT)
			{
				base::dictionary* pdict = new base::dictionary(*account_dict);
				ref_dictionary* rd = new ref_dictionary(pdict);

				(*pdict)["cmd"] = ATPM_CMD_RSP_ACCOUNT;
				(*pdict)["queryid"] = matcher_dict["queryid"].string_value();
				(*pdict)["error_code"] = (int)0;

				atp_message msg;
				msg.type = ATP_MESSAGE_TYPE_QUERY_REPLY;
				msg.param1 = (void*)rd;
				mdpt_->dispatch_message(msg);

				qrd->release();
				it = wait_match_queue_.erase(it);
			}
			else {
				++it;
			}
		}
		delete account_dict;
		map_dict_account_[account_key] = NULL;
	}
	return NAUT_AT_S_OK;
}

int query_processor::process_rsp_openinter(base::dictionary& dict)
{
   std::string account_key = trade_server::get_account_broker_bs_key(dict["broker"].string_value(), dict["account"].string_value());
	base::dictionary* openinter_dict = map_dict_openinter_[account_key];
	if (openinter_dict == NULL) {
		openinter_dict = new base::dictionary();
		(*openinter_dict)["count"] = (int)0;
		(*openinter_dict)["result"] = new base::darray();
		(*openinter_dict)["account"] = dict["account"].string_value();
		(*openinter_dict)["userid"] = dict["userid"].string_value();
		(*openinter_dict)["broker"] = dict["broker"].string_value();
		map_dict_openinter_[account_key] = openinter_dict;
	}

	/* add result record */
	(*openinter_dict)["count"] = (*openinter_dict)["count"].int_value() + 1;
	(*openinter_dict)["result"].array_value()->append(dict);
	if (dict["result_complete"].string_value() != "1") {
		(*openinter_dict)["complete"] = 0;
	}
	else {
		(*openinter_dict)["complete"] = 1;
	}

	/* check whether the result is completed */
	if (dict["result_complete"].string_value() == "1") {
		std::vector<atp_message>::iterator it = wait_match_queue_.begin();
		for (; it != wait_match_queue_.end(); ) {
			atp_message& qmsg = (*it);
			ref_dictionary* qrd = (ref_dictionary*)qmsg.param1;
			base::dictionary& matcher_dict = *(qrd->get());
			if (matcher_dict["cmd"].string_value() == ATPM_CMD_QRY_OPENINTER)
			{
				base::dictionary* pdict = new base::dictionary(*openinter_dict);
				ref_dictionary* rd = new ref_dictionary(pdict);

				(*pdict)["cmd"] = ATPM_CMD_RSP_OPENINTER;
				(*pdict)["queryid"] = matcher_dict["queryid"].string_value();
				(*pdict)["error_code"] = (int)0;

				atp_message msg;
				msg.type = ATP_MESSAGE_TYPE_QUERY_REPLY;
				msg.param1 = (void*)rd;
				mdpt_->dispatch_message(msg);

				qrd->release();
				it = wait_match_queue_.erase(it);
			}
			else {
				++it;
			}
		}
		delete openinter_dict;
		map_dict_openinter_[account_key] = NULL;
	}
	return NAUT_AT_S_OK;
}

int query_processor::process_rsp_entrust(base::dictionary& dict)
{
   std::string account_key = trade_server::get_account_broker_bs_key(dict["broker"].string_value(), dict["account"].string_value());
	base::dictionary* entrust_dict = map_dict_entrust_[account_key];
	if (entrust_dict == NULL) {
		entrust_dict = new base::dictionary();
		(*entrust_dict)["count"] = (int)0;
		(*entrust_dict)["result"] = new base::darray();
		(*entrust_dict)["account"] = dict["account"].string_value();
		(*entrust_dict)["userid"] = dict["userid"].string_value();
		(*entrust_dict)["broker"] = dict["broker"].string_value();
		map_dict_entrust_[account_key] = entrust_dict;
	}

	/* add result record */
	(*entrust_dict)["count"] = (*entrust_dict)["count"].int_value() + 1;
	(*entrust_dict)["result"].array_value()->append(dict);
	if (dict["result_complete"].string_value() != "1") {
		(*entrust_dict)["complete"] = 0;
	}
	else {
		(*entrust_dict)["complete"] = 1;
	}

	/* check whether the result is completed */
	if (dict["result_complete"].string_value() == "1") {
		std::vector<atp_message>::iterator it = wait_match_queue_.begin();
		for (; it != wait_match_queue_.end(); )
		{
			atp_message& qmsg = (*it);
			ref_dictionary* qrd = (ref_dictionary*)qmsg.param1;
			base::dictionary& matcher_dict = *(qrd->get());
			if (matcher_dict["cmd"].string_value() == ATPM_CMD_QRY_CHECKER_ENTRUST)
			{
				bool matched = false;
				int result_count = (*entrust_dict)["count"].int_value();
				for (int i = 0; i < result_count; i++) {
					base::dictionary* record = (base::dictionary*)(*entrust_dict)["result"].array_value()->at(i).dictionary_value();
					std::string localno = (*record)["localno"].string_value();
					if (dict["account"].string_value() == matcher_dict["account"].string_value() &&
						localno == matcher_dict["localno"].string_value()) {
						matched = true;
						break;
					}
				}
				if (!matched) {
					++it;
					continue;
				}

				base::dictionary* pdict = new base::dictionary(*entrust_dict);
				ref_dictionary* rd = new ref_dictionary(pdict);

				(*pdict)["cmd"] = ATPM_CMD_RSP_CHECKER_ENTRUST;
				(*pdict)["queryid"] = matcher_dict["queryid"].string_value();
				(*pdict)["error_code"] = (int)0;

				atp_message msg;
				msg.type = ATP_MESSAGE_TYPE_CHECKER_RSP;
				msg.param1 = (void*)rd;
				mdpt_->dispatch_message(msg);

				qrd->release();
				it = wait_match_queue_.erase(it);
			}
			else {
				++it;
			}
		}
		delete entrust_dict;
		map_dict_entrust_[account_key] = NULL;
	}
	return NAUT_AT_S_OK;
}

int query_processor::process_rsp_checker_entrust(base::dictionary& dict)
{
	return process_rsp_entrust(dict);
}

int query_processor::process_rsp_deal(base::dictionary& dict)
{
   std::string account_key = trade_server::get_account_broker_bs_key(dict["broker"].string_value(), dict["account"].string_value());
	base::dictionary* deal_dict = map_dict_deal_[account_key];

	if (deal_dict == NULL) {
		deal_dict = new base::dictionary();
		(*deal_dict)["count"] = (int)0;
		(*deal_dict)["result"] = new base::darray();
		(*deal_dict)["account"] = dict["account"].string_value();
		(*deal_dict)["userid"] = dict["userid"].string_value();
		(*deal_dict)["broker"] = dict["broker"].string_value();
		map_dict_deal_[account_key] = deal_dict;
	}

	/* add result record */
	(*deal_dict)["count"] = (*deal_dict)["count"].int_value() + 1;
	(*deal_dict)["result"].array_value()->append(dict);
	if (dict["result_complete"].string_value() != "1") {
		(*deal_dict)["complete"] = 0;
	}
	else {
		(*deal_dict)["complete"] = 1;
	}

	/* check whether the result is completed */
	if (dict["result_complete"].string_value() == "1")
	{
		std::vector<atp_message>::iterator it = wait_match_queue_.begin();
		for (; it != wait_match_queue_.end(); ) {
			atp_message& qmsg = (*it);
			ref_dictionary* qrd = (ref_dictionary*)qmsg.param1;
			base::dictionary& matcher_dict = *(qrd->get());
			if (matcher_dict["cmd"].string_value() == ATPM_CMD_QRY_DEAL ||
				matcher_dict["cmd"].string_value() == ATPM_CMD_QRY_CHECKER_DEAL)
			{
				bool matched = false;
				int result_count = (*deal_dict)["count"].int_value();
				for (int i = 0; i < result_count; i++) {
					base::dictionary* record = (base::dictionary*)(*deal_dict)["result"].array_value()->at(i).dictionary_value();
					std::string systemno = (*record)["systemno"].string_value();
					if (dict["account"].string_value() == matcher_dict["account"].string_value() &&
						systemno == matcher_dict["systemno"].string_value()) {
						matched = true;
						break;
					}
				}
				if (!matched) {
					++it;
					continue;
				}

				(*deal_dict)["cmd"] = ATPM_CMD_RSP_DEAL;
				if (matcher_dict["cmd"].string_value() == ATPM_CMD_QRY_CHECKER_DEAL) {
					(*deal_dict)["cmd"] = ATPM_CMD_RSP_CHECKER_DEAL;
				}

				base::dictionary* pdict = new base::dictionary(*deal_dict);

				/* check and remove invalid record */
				int count = (*pdict)["count"].int_value();
				for (int i = count - 1; i >= 0; i--) {
					base::dictionary* record = (base::dictionary*)(*pdict)["result"].array_value()->at(i).dictionary_value();
					if ((*record)["code"].string_value().length() < 2) {
						(*pdict)["result"].array_value()->remove(i);
						(*pdict)["count"] = (*pdict)["count"].int_value() - 1;
					}
				}
				if ((*pdict)["count"].int_value() == 0) {
					pdict->remove_object("result");
				}

				ref_dictionary* rd = new ref_dictionary(pdict);
				atp_message msg;
				msg.type = ATP_MESSAGE_TYPE_QUERY_REPLY;
				if ((*pdict)["cmd"].string_value() == ATPM_CMD_RSP_CHECKER_DEAL) {
					msg.type = ATP_MESSAGE_TYPE_CHECKER_RSP;
				}
				(*pdict)["queryid"] = matcher_dict["queryid"].string_value();
				(*pdict)["orderid"] = matcher_dict["orderid"].string_value();
				(*pdict)["entrust_type"] = matcher_dict["entrust_type"].int_value();
				(*pdict)["direction"] = matcher_dict["direction"].int_value();
				(*pdict)["error_code"] = (int)0;

				TRACE_DEBUG(AT_TRACE_TAG, "deal query response, %s (%s)",
						(*pdict)["orderid"].string_value().c_str(), pdict->to_string().c_str());

				msg.param1 = (void*)rd;
				mdpt_->dispatch_message(msg);

				qrd->release();
				it = wait_match_queue_.erase(it);
			}
			else {
				++it;
			}
		}
		delete deal_dict;
		map_dict_deal_[account_key] = NULL;
	}
	return NAUT_AT_S_OK;
}

int query_processor::process_rsp_checker_deal(base::dictionary& dict)
{
	return process_rsp_deal(dict);
}

int query_processor::process_cmd_error(base::dictionary& dict)
{
	assert(dict["error_code"].int_value() != 0);

	bool need_reply = true;
	std::string cmd = dict["real_cmd"].string_value();
	std::string queryid = dict["queryid"].string_value();

	std::vector<atp_message>::iterator it = wait_match_queue_.begin();
	for ( ; it != wait_match_queue_.end(); it++) {
		ref_dictionary* matcher_rd = (ref_dictionary*)((*it).param1);
		base::dictionary* matcher_dict = matcher_rd->get();
		if (queryid == (*matcher_dict)["queryid"].string_value()) {
			wait_match_queue_.erase(it);
			break;
		}
	}

	if (cmd == ATPM_CMD_QRY_ACCOUNT) {
		cmd = ATPM_CMD_RSP_ACCOUNT;
	}
	else if (cmd == ATPM_CMD_QRY_DEAL) {
		cmd = ATPM_CMD_RSP_DEAL;
	}
	else if (cmd == ATPM_CMD_QRY_OPENINTER) {
		cmd = ATPM_CMD_RSP_OPENINTER;
	}
	else {
		need_reply = false;
	}

	if (need_reply)
	{
		base::dictionary* pdict = new base::dictionary(dict);
		ref_dictionary* rd = new ref_dictionary(pdict);

		(*pdict)["cmd"] = cmd;
		(*pdict)["count"] = (int)0;
		pdict->remove_object("real_cmd");

		atp_message msg;
		msg.type = ATP_MESSAGE_TYPE_ORDER_REPLY;
		msg.param1 = (void*)rd;
		mdpt_->dispatch_message(msg);
	}
	return NAUT_AT_S_OK;
}

void query_processor::release_messages()
{
	atp_message msg;
	while (get(msg) == 0) {
		ref_dictionary* rd = (ref_dictionary*)msg.param1;
		if (rd != NULL) {
			rd->release();
		}
	}
	for (int i = 0; i < (int)wait_match_queue_.size(); i++) {
		ref_dictionary* rd = (ref_dictionary*)wait_match_queue_[i].param1;
		if (rd != NULL) {
			rd->release();
		}
	}
	wait_match_queue_.clear();
}

void query_processor::check_query_wait_timeout()
{
	std::vector<atp_message>::iterator it = wait_match_queue_.begin();
	for ( ; it != wait_match_queue_.end(); ) {
		long timeout = (long)(*it).param2;
		if (base::util::clock() > timeout)
		{
			ref_dictionary* matcher_rd = (ref_dictionary*)(*it).param1;
			std::string cmd = (*(matcher_rd->get()))["cmd"].string_value();

			if (cmd == ATPM_CMD_QRY_CHECKER_ENTRUST || cmd == ATPM_CMD_QRY_CHECKER_DEAL) {
				/* nothing needs to do */
			}
			else {
				base::dictionary* pdict = new base::dictionary(*(matcher_rd->get()));
				ref_dictionary* rd = new ref_dictionary(pdict);

				if (cmd == ATPM_CMD_QRY_ACCOUNT) {
					(*pdict)["cmd"] = ATPM_CMD_RSP_ACCOUNT;
				}
				else if (cmd == ATPM_CMD_QRY_OPENINTER) {
					(*pdict)["cmd"] = ATPM_CMD_RSP_OPENINTER;
				}
				else if (cmd == ATPM_CMD_QRY_ENTRUST) {
					(*pdict)["cmd"] = ATPM_CMD_RSP_ENTRUST;
				}
				else if (cmd == ATPM_CMD_QRY_DEAL) {
					(*pdict)["cmd"] = ATPM_CMD_RSP_DEAL;
				}
				else if (cmd == ATPM_CMD_QRY_DEAL_ALL) {
					(*pdict)["cmd"] = ATPM_CMD_RSP_DEAL_ALL;
				}
				(*pdict)["count"] = int(0);
                (*pdict)["error_code"] = AT_ERROR_QUERY_TIMEOUT;
                (*pdict)["error_msg"] = get_response_error_msg(AT_ERROR_QUERY_TIMEOUT);

				atp_message msg;
				msg.type = ATP_MESSAGE_TYPE_QUERY_REPLY;
				msg.param1 = rd;
				mdpt_->dispatch_message(msg);
			}

			matcher_rd->release();
			it = wait_match_queue_.erase(it);
		}
		else {
			it++;
		}
	}
}

}
