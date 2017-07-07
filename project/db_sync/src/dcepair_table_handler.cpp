/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: dcepair_table_handler.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#include "dcepair_table_handler.h"
#include "database/dbscope.h"
#include "project_server.h"
#include "common.h"

namespace db_sync{

int dcepair_table_handler::oracle_to_mysql()
{
    TRACE_SYSTEM(MODULE_NAME, "dcepair_table_handler begin");
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
        dcepair_inscom_table_str_.c_str());
    if (!mysql_dbconn->_conn->execute(sql)) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_DB_FAILED,
            "delete dcepair_inscom_table_str_ %s failed", sql);
        return DBSYNC_E_DB_FAILED;
    }

    sprintf(sql, "select * from %s",
            get_project_server().get_process_param().
            oracle_dcepair_inscom_table_str_.c_str());

    if (oracle_dbconn->_conn->query(sql)) {
        while (oracle_dbconn->_conn->fetch_row()) {
            sprintf(sql, "INSERT INTO %s (comkind,comname,contractcode,"
                "leftcont,rightcont,productcode,leftprodcode,rightprodcode,productkind,"
                "exchangecode,permargin,margindir,mainseq,groupseq,contractseq,"
                "groupid,remark) "
                "VALUES ('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',"
                "%f,'%c',%ld,%ld,%ld,%ld,'%s')",
                get_project_server().get_process_param().
                dcepair_inscom_table_str_.c_str(),
                gbk_to_utf8(oracle_dbconn->_conn->get_string("comkind")).c_str(),
                gbk_to_utf8(oracle_dbconn->_conn->get_string("comname")).c_str(),
                oracle_dbconn->_conn->get_string("contractcode"),
                oracle_dbconn->_conn->get_string("leftcont"),
                oracle_dbconn->_conn->get_string("rightcont"),
                oracle_dbconn->_conn->get_string("productcode"),
                oracle_dbconn->_conn->get_string("leftprodcode"),
                oracle_dbconn->_conn->get_string("rightprodcode"),
                oracle_dbconn->_conn->get_string("productkind"),
                oracle_dbconn->_conn->get_string("exchangecode"),
                oracle_dbconn->_conn->get_double("permargin"),
                oracle_dbconn->_conn->get_string("margindir")[0],
                oracle_dbconn->_conn->get_long("mainseq"),
                oracle_dbconn->_conn->get_long("groupseq"),
                oracle_dbconn->_conn->get_long("contractseq"),
                oracle_dbconn->_conn->get_long("groupid"),
                oracle_dbconn->_conn->get_string("remark"));
            if (!mysql_dbconn->_conn->execute(sql)) {
                TRACE_ERROR(MODULE_NAME, DBSYNC_E_DB_FAILED,
                    "add dcepair_inscom_table_str_ %s failed", sql);
                return DBSYNC_E_DB_FAILED;
            }
        }
    }
    TRACE_SYSTEM(MODULE_NAME, "dcepair_table_handler end");
    return ret;
}

}// db_sync
