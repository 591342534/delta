/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: db_deposit_handler.cpp
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#include "parse.h"
#include "db_deposit_handler.h"
#include "database/dbscope.h"
#include "project_server.h"
#include "common.h"
#include "base/log_binder.h"
////////////////////////////////////////////////////////////////////////////////
using namespace common;
namespace serverframe{;

////////////////////////////////////////////////////////////////////////////////
void db_deposit_handler::on_request(Context& context)
{
    try {
        std::string message;
        unsigned int clientid;
        decode(context, message, clientid);

        customer_cash_deposit data;
        memcpy(&data, message.c_str(), sizeof(customer_cash_deposit));

        TRACE_INFO(MODULE_NAME, "db_deposit_handler::on_request, clientid:%d "
                "customer:%s", clientid, data.customer);

        //database::dbscope mysql_db_keepper(
        //        get_project_server().get_db_conn_pool());
        //database::db_instance* mysql_dbconn = mysql_db_keepper.get_db_conn();
        ////CHECK_IF_DBCONN_NULL(mysql_dbconn);

        //char sql[1024] = { 0 };
        //sprintf(sql, "insert %s(calc_time, customer, cffex, czce, dce, shfe, total) "
        //        "VALUES(FROM_UNIXTIME(%ld), '%s', %f, %f, %f, %f, %f)",
        //        get_project_server().get_process_param().calcdeposit_table_str_.c_str(),
        //        data.time_stamps, data.customer,
        //        data.cffex, data.czce, data.dce, data.shfe, data.total);
        //if(!mysql_dbconn->_conn->execute(sql)) {
        //    throw 404;
        //}
    }
    catch (int error_code) {
        TRACE_ERROR(MODULE_NAME, MANAGER_E_DB_FAILED, \
                "qry_insmrgnrate_handler failed to on_request, error:%d", error_code);
    }
}

////////////////////////////////////////////////////////////////////////////////
}// serverframe

