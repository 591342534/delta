/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: instrument_table_handler.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#include "instrument_table_handler.h"
#include "database/dbscope.h"
#include "project_server.h"
#include "common.h"

namespace db_sync{

int instrument_table_handler::oracle_to_mysql()
{
    TRACE_SYSTEM(MODULE_NAME, "instrument_table_handler begin");
    int ret = true;
    database::dbscope oracle_db_keepper(
            get_project_server().get_oracle_db_pool());
    database::db_instance* oracle_dbconn = oracle_db_keepper.get_db_conn();
    CHECK_IF_DBCONN_NULL(oracle_dbconn);

    database::dbscope mysql_db_keepper(
            get_project_server().get_mysql_db_pool());
    database::db_instance* mysql_dbconn = mysql_db_keepper.get_db_conn();
    CHECK_IF_DBCONN_NULL(mysql_dbconn);

    char sql[1024] = { 0 };
    sprintf(sql, "DELETE from %s",
        get_project_server().get_process_param().
        instrument_table_str_.c_str());
    if (!mysql_dbconn->_conn->execute(sql)) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_DB_FAILED,
            "delete t_instrument %s failed", sql);
        return DBSYNC_E_DB_FAILED;
    }

    sprintf(sql, "select * from %s",
            get_project_server().get_process_param().
        oracle_instrument_table_str_.c_str());

    if (oracle_dbconn->_conn->query(sql)) {
        while (oracle_dbconn->_conn->fetch_row()) {
            sprintf(sql, "INSERT INTO %s (instrumentid,exchangeid,instrumentname,"
                    "exchangeinstid,productid,productclass,openyear,openmonth,"
                    "deliveryyear,deliverymonth,maxmarketordervolume,minmarketordervolume,"
                    "maxlimitordervolume,minlimitordervolume,volumemultiple,pricetick,"
                    "createdate,opendate,expiredate,instlifephase,istrading,positiontype,"
                    "positiondatetype,deliverymode,delivsmarginratebymoney,delivsmarginratebyvolume,"
                    "delivhmarginratebymoney,delivhmarginratebyvolume,optionstype,"
                    "strikeprice) "
                "VALUES ('%s','%s','%s','%s','%s','%c',%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%f,"
                "'%s','%s','%s','%c',%ld,'%c','%c','%c',%f,%f,%f,%f,'%s',%f)",
                get_project_server().get_process_param().
                instrument_table_str_.c_str(),
                oracle_dbconn->_conn->get_string("INSTRUMENTID"),
                oracle_dbconn->_conn->get_string("EXCHANGEID"), 
                oracle_dbconn->_conn->get_string("INSTRUMENTNAME"),
                oracle_dbconn->_conn->get_string("EXCHANGEINSTID"),
                oracle_dbconn->_conn->get_string("PRODUCTID"),
                oracle_dbconn->_conn->get_string("PRODUCTCLASS")[0],
                oracle_dbconn->_conn->get_long("OPENYEAR"),
                oracle_dbconn->_conn->get_long("OPENMONTH"),
                oracle_dbconn->_conn->get_long("DELIVERYYEAR"),
                oracle_dbconn->_conn->get_long("DELIVERYMONTH"),
                oracle_dbconn->_conn->get_long("MAXMARKETORDERVOLUME"),
                oracle_dbconn->_conn->get_long("MINMARKETORDERVOLUME"),
                oracle_dbconn->_conn->get_long("MAXLIMITORDERVOLUME"),
                oracle_dbconn->_conn->get_long("MINLIMITORDERVOLUME"),
                oracle_dbconn->_conn->get_long("VOLUMEMULTIPLE"),
                oracle_dbconn->_conn->get_double("PRICETICK"),
                oracle_dbconn->_conn->get_string("CREATEDATE"),
                oracle_dbconn->_conn->get_string("OPENDATE"),
                oracle_dbconn->_conn->get_string("EXPIREDATE"),
                oracle_dbconn->_conn->get_string("INSTLIFEPHASE")[0],
                oracle_dbconn->_conn->get_long("ISTRADING"),
                oracle_dbconn->_conn->get_string("POSITIONTYPE")[0],
                oracle_dbconn->_conn->get_string("POSITIONDATETYPE")[0],
                oracle_dbconn->_conn->get_string("DELIVERYMODE")[0],
                oracle_dbconn->_conn->get_double("DELIVSMARGINRATEBYMONEY"),
                oracle_dbconn->_conn->get_double("DELIVSMARGINRATEBYVOLUME"),
                oracle_dbconn->_conn->get_double("DELIVHMARGINRATEBYMONEY"),
                oracle_dbconn->_conn->get_double("DELIVHMARGINRATEBYVOLUME"),
                oracle_dbconn->_conn->get_string("OPTIONSTYPE"),
                oracle_dbconn->_conn->get_double("STRIKEPRICE"));
            if (!mysql_dbconn->_conn->execute(sql)) {
                TRACE_ERROR(MODULE_NAME, DBSYNC_E_DB_FAILED,
                    "add t_instrument %s failed", sql);
                return DBSYNC_E_DB_FAILED;
            }
        }
    }
    TRACE_SYSTEM(MODULE_NAME, "instrument_table_handler end");
    return ret;
}

}// db_sync
