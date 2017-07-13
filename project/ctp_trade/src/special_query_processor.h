/*****************************************************************************
 Nautilus Module stock_trade Copyright (c) 2016. All Rights Reserved.

 FileName: special_query_processor.h
 Version: 1.0
 Date: 2016.03.21

 History:
 david wang     2016.03.21   1.0     Create
 ******************************************************************************/

#ifndef __NAUT_STOCKTRADE_SPECIAL_QUERY_PROCESSOR_H__
#define __NAUT_STOCKTRADE_SPECIAL_QUERY_PROCESSOR_H__

#include "processor_base.h"
#include "common.h"
#include "trade_unit.h"
#include "base/dictionary.h"
#include "base/event.h"
#include "base/file.h"
#include "database/unidbpool.h"

namespace ctp
{

class trade_server;

class special_query_processor
	: public processor_base
{
public:
	special_query_processor();
	virtual ~special_query_processor();

public:
	int start(trade_server* tserver, message_dispatcher* mdpt);

public:
	virtual void stop();

protected:
	int start_internal();
	int stop_internal();

	virtual void run();

	trade_unit* is_account_exist(const char* account);

	int prepare_qrydeal();

private:
	trade_server* tserver_;
	message_dispatcher* mdpt_;
	bool started_;
	long m_last_deal_id_;
	int deal_id_file_;
	string last_deal_id_file;
};

}

#endif  //__NAUT_STOCKTRADE_SPECIAL_QUERY_PROCESSOR_H__
