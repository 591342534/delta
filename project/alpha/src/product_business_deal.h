/*****************************************************************************
data_query_manager Copyright (c) 2016. All Rights Reserved.

FileName: product_business_deal.h
Version: 1.0
Date: 2016.11.1

History:
ericsheng     2016.11.1   1.0     Create
******************************************************************************/

#ifndef _PRODUCT_BUSINESS_DEAL_H_
#define _PRODUCT_BUSINESS_DEAL_H_

#include <string>
#include "base/dictionary.h"
#include "common.h"
#include "sqlmake.h"

namespace serverframe
{

class product_business_deal
{
public:
    product_business_deal();
    ~product_business_deal();

protected:
    std::string make_sqlcondition_prefix(std::string &current);
    std::string make_condition_dict_str(base::dictionary &dict, const std::string &fieldname, const std::string &tablename="");
	std::string make_sqlcondition(std::string &current, const std::string &append);
	std::string make_sqlcondition(std::string &current, base::dictionary &dict, const std::string &fieldname, const std::string &tablename="");
	std::string make_sql_date(std::string &current, base::dictionary &dict, const std::string &fieldname, const std::string &tablename="");
	std::string make_sql_from_date(std::string &current, base::dictionary &dict, const std::string &fieldname, const std::string &tablename="");
	std::string make_sql_end_date(std::string &current, base::dictionary &dict, const std::string &fieldname, const std::string &tablename="");
	std::string make_sql_startindex(std::string &current, base::dictionary &dict, const std::string &tablename="");
	std::string make_sql_limit(base::dictionary &dict);
	void make_condition_field(sqlmaker &sql, const base::dictionary &dict, const std::string &fieldname, sqlmaker::datatype type);
	void make_update_field(sqlmaker &sql, const base::dictionary &dict, const std::string &fieldname, sqlmaker::datatype type);
	void set_sql_error_response(base::dictionary &dict, int nerrno, const std::string &error);
	bool query_sql_check(const std::string &sql);

public:
	//查询
	int sql_query(base::dictionary& indict, base::dictionary& outdict);
    int account_fund_query(base::dictionary& indict, base::dictionary& outdict);

	int trade_system_config_insert(base::dictionary& indict, base::dictionary& outdict);
	int trade_system_config_update(base::dictionary& indict, base::dictionary& outdict);
	int trade_system_config_delete(base::dictionary& indict, base::dictionary& outdict);

};

}

#endif //_PRODUCT_BUSINESS_DEAL_H_
