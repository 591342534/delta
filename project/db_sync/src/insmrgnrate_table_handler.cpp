/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: insmrgnrate_table_handler.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#include "insmrgnrate_table_handler.h"
#include "database/dbscope.h"
#include "project_server.h"
#include "common.h"

namespace db_sync{

int insmrgnrate_table_handler::oracle_to_mysql()
{
    TRACE_SYSTEM(MODULE_NAME, "insmrgnrate_table_handler begin");
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
        insmrgnrate_table_str_.c_str());
    if (!mysql_dbconn->_conn->execute(sql)) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_DB_FAILED,
            "delete insmrgnrate_table_str_ %s failed", sql);
        return DBSYNC_E_DB_FAILED;
    }

    sprintf(sql, "select * from %s",
            get_project_server().get_process_param().
            oracle_insmrgnrate_table_str_.c_str());

    if (oracle_dbconn->_conn->query(sql)) {
        while (oracle_dbconn->_conn->fetch_row()) {
            //cout << oracle_dbconn->_conn->get_string("tmplname") << endl;
            sprintf(sql, "INSERT INTO %s (tradingday,contractcode,exchangecode,"
                "hedgeflag,direction,marginrate,segmrgn,tmplname,marginexpr,ismanual,remark) "
                "VALUES ('%s','%s','%s','%c','%c',%f,%f,'%s','%s','%c','%s')",
                get_project_server().get_process_param().
                insmrgnrate_table_str_.c_str(),
                oracle_dbconn->_conn->get_string("tradingday"),
                oracle_dbconn->_conn->get_string("contractcode"),
                oracle_dbconn->_conn->get_string("exchangecode"),
                oracle_dbconn->_conn->get_string("hedgeflag")[0],
                oracle_dbconn->_conn->get_string("direction")[0],
                oracle_dbconn->_conn->get_double("marginrate"),
                oracle_dbconn->_conn->get_double("segmrgn"),
                gbk_to_utf8(oracle_dbconn->_conn->get_string("tmplname")).c_str(),
                gbk_to_utf8(oracle_dbconn->_conn->get_string("marginexpr")).c_str(),
                oracle_dbconn->_conn->get_string("ismanual")[0],
                gbk_to_utf8(oracle_dbconn->_conn->get_string("remark")).c_str());
            if (!mysql_dbconn->_conn->execute(sql)) {
                TRACE_ERROR(MODULE_NAME, DBSYNC_E_DB_FAILED,
                    "add insmrgnrate_table_str_ %s failed", sql);
                return DBSYNC_E_DB_FAILED;
            }
        }
    }
    TRACE_SYSTEM(MODULE_NAME, "insmrgnrate_table_handler end");
    return ret;
}

}// db_sync
