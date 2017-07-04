/*****************************************************************************
data_query_manager Copyright (c) 2016. All Rights Reserved.

FileName: http_deal.h
Version: 1.0
Date: 2016.11.1

History:
sky     2016.11.1   1.0     Create
******************************************************************************/

#ifndef HTTP_DEAL_H_
#define HTTP_DEAL_H_

#include "process.h"
#include "common.h"
#include "errordef.h"
#include "base/http_server.h"
namespace serverframe
{
class http_deal
{
public:
	http_deal();
	virtual ~http_deal();

public:
	int service_onoff(evkeyvalq *output_Headers, evkeyvalq &mncg_http_query, evbuffer *evb);

	//查询
	int sql_query(evkeyvalq *output_Headers, base::dictionary *ptr_dict, evbuffer *evb);
	int account_fund_query(evkeyvalq *output_Headers, base::dictionary *ptr_dict, evbuffer *evb);

	int trade_system_config_insert(evkeyvalq *output_Headers, base::dictionary *ptr_dict, evbuffer *evb);
	int trade_system_config_update(evkeyvalq *output_Headers, base::dictionary *ptr_dict, evbuffer *evb);
	int trade_system_config_delete(evkeyvalq *output_Headers, base::dictionary *ptr_dict, evbuffer *evb);


	int result();
	std::string result_msg();
private:
	int m_nresult;
	std::string m_result_msg;
};

}

#endif /* HTTP_DEAL_H_ */
