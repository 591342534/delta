/*****************************************************************************
 Nautilus Module stock_trade Copyright (c) 2016. All Rights Reserved.

 FileName: special_query_processor.cpp
 Version: 1.0
 Date: 2016.03.21

 History:
 david wang     2016.03.21   1.0     Create
 ******************************************************************************/
#include "special_query_processor.h"
#include "common.h"
#include "trade_server.h"
#include "base/trace.h"
#include "base/util.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PROCESS_COMPLETE_FLAG -1

namespace ctp
{

special_query_processor::special_query_processor()
	: tserver_(NULL)
	, mdpt_(NULL)
	, started_(false)
	, m_last_deal_id_(0)
   , deal_id_file_(0)
{
    last_deal_id_file = "./last_deal_id.txt";
    base::file* dealfile = new base::file();
    if(dealfile->open(last_deal_id_file.c_str()) != 0) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_OPEN_PROGRESS_LOG_FILE_FAILED,
                "open for read last_deal_id.txt failed~~~");
    } else {
        char outbuf[30];
        int outlen = 30;
        memset(outbuf, 0, outlen);
        if(dealfile->read((base::byte*)outbuf, outlen) > 0) {
            sscanf(outbuf, "%ld", &m_last_deal_id_);
        }
    }
    dealfile->close();
    delete dealfile;
    dealfile = NULL;

    deal_id_file_ = open(last_deal_id_file.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if(deal_id_file_ == 0) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_OPEN_PROGRESS_LOG_FILE_FAILED,
                "open for write last_deal_id.txt failed~~~");
    }
}

special_query_processor::~special_query_processor()
{
	stop();
}

int special_query_processor::start(trade_server* tserver, message_dispatcher* mdpt)
{
	assert(tserver != NULL);
	assert(mdpt != NULL);

	if (started_) {
		return NAUT_AT_S_OK;
	}

	tserver_ = tserver;
	mdpt_ = mdpt;

	int ret = start_internal();
	if (BSUCCEEDED(ret)) {
		started_ = true;
	}
	else {
		stop();
	}
	return ret;
}

void special_query_processor::stop()
{
	started_ = false;
	stop_internal();
}

int special_query_processor::start_internal()
{
	/* start processor thread */
	processor_base::start();

	return NAUT_AT_S_OK;
}

int special_query_processor::stop_internal()
{
	processor_base::stop();

	if(deal_id_file_ ) {
		close(deal_id_file_);
		deal_id_file_ = NULL;
	}

	return NAUT_AT_S_OK;
}

void special_query_processor::run()
{
	while (is_running_)
	{
	    if(tserver_->get_server_start_flag()) {
            prepare_qrydeal();
	    }
        base::thread::sleep(1000);
	}
}

trade_unit* special_query_processor::is_account_exist(const char* account)
{
	map_str_trade_unit& maptunits = tserver_->map_tunits();
	map_str_trade_unit::const_iterator con_iter = maptunits.find(account);
	if (con_iter != maptunits.end()) {
			return con_iter->second;
	}

	return NULL;
}


int special_query_processor::prepare_qrydeal()
{
	int ret = NAUT_AT_S_OK;
    naut::DBCONNECT* m_db_conn_ = NULL;

    LABEL_SCOPE_START;
    m_db_conn_ = tserver_->get_risk_conn_pool()->getconn();

    if(m_db_conn_ == NULL) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_INIT_DATABASE_CONN_FAILED,
                        "get_conn_pool failed: ~~~");
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_INIT_DATABASE_CONN_FAILED, end);
    }


	/* query deal */
	char sql[512];
	sprintf(sql, "select  dealid, broker, account, entrustno, dealno, entrustbs,"
			" stockcode, stockname, dealprice, dealamount, dealfund, dealtime, dealresult, "
			"reportno, entrustid, recordtime from %s where dealid > %ld",tserver_->get_autotrade_deal_tbl_name().c_str(), m_last_deal_id_);
	if (!m_db_conn_->_conn->query(sql)) {
		TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_QUERY_SERVER_CONFIG_FAILED,
				"query server config failed, db error: (%d:%s)", m_db_conn_->_conn->get_errno(), m_db_conn_->_conn->get_error().c_str());
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_QUERY_SERVER_CONFIG_FAILED, end);
	}

	if (m_db_conn_->_conn->get_count() == 0) {
		TRACE_DEBUG(AT_TRACE_TAG, "There do not have new deals now to handle!!!");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_S_OK, end);
	}
	while (m_db_conn_->_conn->fetch_row()) {
	     std::string account_key = tserver_->get_account_broker_bs_key(m_db_conn_->_conn->get_string("broker"), m_db_conn_->_conn->get_string("account"));
        trade_unit* tradeu = is_account_exist(account_key.c_str());
        if(tradeu != NULL) {
            base::dictionary dict;

            // entrust_type   direction entrustbs
            // 1    1    1
            // 1    2    2
            // 2    1    3
            // 2    2    4
            switch(m_db_conn_->_conn->get_long("entrustbs")) {
                case 1:
                    dict["entrust_type"] = (long)1;
                    dict["direction"] = (long)1;
                    break;
                case 2:
                    dict["entrust_type"] = (long)1;
                    dict["direction"] = (long)2;
                    break;
                case 3:
                    dict["entrust_type"] = (long)2;
                    dict["direction"] = (long)1;
                    break;
                case 4:
                    dict["entrust_type"] = (long)2;
                    dict["direction"] = (long)2;
                    break;
                default:
                    TRACE_WARNING(AT_TRACE_TAG, "entrustbs:%ld is not right", m_db_conn_->_conn->get_long("entrustbs"));
                    break;
            }

            dict["broker"] = m_db_conn_->_conn->get_string("broker");
            dict["account"] = m_db_conn_->_conn->get_string("account");
            dict["entrustno"] = (long) m_db_conn_->_conn->get_long("entrustno");
            dict["dealno"] = m_db_conn_->_conn->get_string("dealno");
            dict["systemno"] = m_db_conn_->_conn->get_string("entrustid");
            dict["code"] = m_db_conn_->_conn->get_string("stockcode");
            dict["code_name"] = m_db_conn_->_conn->get_string("stockname");
            dict["deal_amount"] = (long) m_db_conn_->_conn->get_long("dealamount");
            dict["deal_price"] = (double) m_db_conn_->_conn->get_double("dealprice");
            dict["deal_date"] = base::util::local_date_string();
            dict["deal_time"] = m_db_conn_->_conn->get_string("dealtime");
            dict["deal_fund"] = (double)m_db_conn_->_conn->get_double("dealfund");
            string dealresult = m_db_conn_->_conn->get_string("dealresult");
            if(dealresult == "1") {
                dict["at_error"] = (long)AT_ERROR_NONE;
                if(dict["deal_fund"].double_value() <= 0.0 || dict["deal_amount"].int_value() <=0 || dict["deal_price"].double_value() <= 0.0){
                    TRACE_WARNING(AT_TRACE_TAG, "deal_fund:%lf, deal_amount:%ld, deal_price:%lf must have zero!!!",
                            dict["deal_fund"].double_value(), dict["deal_amount"].int_value(), dict["deal_price"].double_value());
                    ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_S_OK, end);
                }
            } else if (dealresult == "2") {
                dict["at_error"] = AT_ERROR_ORDER_HAS_BEEN_WITHDRAWED_OR_IS_FAILED;
            }

            TRACE_DEBUG(AT_TRACE_TAG, "get deal:%s prepare send now~", dict.to_string().c_str());
            ret = tradeu->process_entrust_deal_result(dict);
            if(ret != NAUT_AT_S_OK) {
                ASSIGN_AND_CHECK_LABEL(ret, ret, end);
            } else {
                m_last_deal_id_ = m_db_conn_->_conn->get_long("dealid");
                string str_last_deal_id = m_db_conn_->_conn->get_string("dealid");
                if(deal_id_file_ != 0) {
                    ftruncate(deal_id_file_, 0);
                    lseek(deal_id_file_, 0, SEEK_SET);
                    write(deal_id_file_, str_last_deal_id.c_str(), str_last_deal_id.length());
                        }
                }
        }
	}

	LABEL_SCOPE_END;
end:
    if(m_db_conn_ != NULL) {
         tserver_->get_risk_conn_pool()->retconn(m_db_conn_);
    }
	return ret;
}

}
