/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: calendar_table_handler.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#include "calendar_table_handler.h"
#include "database/dbscope.h"
#include "project_server.h"
#include "common.h"

namespace db_sync{

int calendar_table_handler::oracle_to_mysql()
{
    TRACE_SYSTEM(MODULE_NAME, "calendar_table_handler begin");
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
        calendar_table_str_.c_str());
    if (!mysql_dbconn->_conn->execute(sql)) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_DB_FAILED,
            "delete calendar_table_str_ %s failed", sql);
        return DBSYNC_E_DB_FAILED;
    }

    sprintf(sql, "select * from %s",
            get_project_server().get_process_param().
            oracle_calendar_table_str_.c_str());

    if (oracle_dbconn->_conn->query(sql)) {
        while (oracle_dbconn->_conn->fetch_row()) {
            sprintf(sql, "INSERT INTO %s (day,dat,wrk,"
                "tra,sun,mon,tue,wed,thu,fri,sat,str,tal,spr) "
                "VALUES ('%s',%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld,"
                "%ld,%ld,%ld,%ld,%ld)",
                get_project_server().get_process_param().
                calendar_table_str_.c_str(),
                oracle_dbconn->_conn->get_string("DAY"),
                oracle_dbconn->_conn->get_long("DAT"),
                oracle_dbconn->_conn->get_long("WRK"),
                oracle_dbconn->_conn->get_long("TRA"),
                oracle_dbconn->_conn->get_long("SUN"),
                oracle_dbconn->_conn->get_long("MON"),
                oracle_dbconn->_conn->get_long("TUE"),
                oracle_dbconn->_conn->get_long("WED"),
                oracle_dbconn->_conn->get_long("THU"),
                oracle_dbconn->_conn->get_long("FRI"),
                oracle_dbconn->_conn->get_long("SAT"),
                oracle_dbconn->_conn->get_long("STR"),
                oracle_dbconn->_conn->get_long("TAL"),
                oracle_dbconn->_conn->get_long("SPR"));
            if (!mysql_dbconn->_conn->execute(sql)) {
                TRACE_ERROR(MODULE_NAME, DBSYNC_E_DB_FAILED,
                    "add calendar_table_str_ %s failed", sql);
                return DBSYNC_E_DB_FAILED;
            }
        }
    }
    TRACE_SYSTEM(MODULE_NAME, "calendar_table_handler end");
    return ret;
}

}// db_sync
