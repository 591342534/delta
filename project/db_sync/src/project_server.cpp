/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: project_server.cpp
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#include "project_server.h"
#include "database/dbscope.h"
#include "base/pugixml.hpp"
#include "common.h"
using namespace database;

namespace db_sync {

void project_server::start(std::string &path)
{
    load_config(path.c_str());

    mysql_db_pool_.init(params_.mysql_dbinfo_.dbparam, 5, params_.mysql_dbinfo_.dbtype);
    oracle_db_pool_.init(params_.oracle_dbinfo_.dbparam, 5, params_.oracle_dbinfo_.dbtype);
    m_app_server.run();
}

void project_server::join()
{
    m_app_server.join();
}

void project_server::stop()
{
    m_app_server.stop();
}

int project_server::load_config(const char* config_file)
{
    int ret = DB_SYNC_S_OK;

    LABEL_SCOPE_START;

    pugi::xml_document config;
    if (!config.load_file(config_file)) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIGFILE_INVALID,
                "xml %s load failed", config_file);
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIGFILE_INVALID, end);
    }

    pugi::xml_node xdb_mysql = config.child("db_sync").child("db_mysql");
    pugi::xml_node xdb_oracle = config.child("db_sync").child("db_oracle");
    pugi::xml_node xtableconfig = config.child("db_sync").child(
            "table_config");
    pugi::xml_node xoracle_tableconfig = config.child("db_sync").child(
            "oracle_table_config");
    //pugi::xml_node xsystem = config.child("db_sync").child("system");
    pugi::xml_node xsync_time = config.child("db_sync").child(
            "sync_time");

    std::string tmp;
    //db_mysql info
    tmp = xdb_mysql.child("type").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: type of the db should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.mysql_dbinfo_.dbtype = tmp;

    tmp = xdb_mysql.child("host").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: host of the db should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.mysql_dbinfo_.dbparam.host = tmp;

    tmp = xdb_mysql.child("port").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: port of the db should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.mysql_dbinfo_.dbparam.port = atoi(tmp.c_str());

    tmp = xdb_mysql.child("user").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: user of the db should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.mysql_dbinfo_.dbparam.user = tmp;

    tmp = xdb_mysql.child("pwd").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: pwd of the db should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.mysql_dbinfo_.dbparam.password = tmp;

    tmp = xdb_mysql.child("dbname").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: dbname of the db should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.mysql_dbinfo_.dbparam.database_name = tmp;

    tmp = xdb_mysql.child("charset").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: charset of the db should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.mysql_dbinfo_.dbparam.charset = tmp;
    params_.mysql_dbinfo_.dbparam.create_database_if_not_exists = false;
    params_.mysql_dbinfo_.dbparam.recreate_database_if_exists = false;

    //db_oracle info
    tmp = xdb_oracle.child("type").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: type of the xdb_oracle should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.oracle_dbinfo_.dbtype = tmp;

    tmp = xdb_oracle.child("host").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: host of the xdb_oracle should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.oracle_dbinfo_.dbparam.host = tmp;

    tmp = xdb_oracle.child("port").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: port of the xdb_oracle should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.oracle_dbinfo_.dbparam.port = atoi(tmp.c_str());

    tmp = xdb_oracle.child("user").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: user of the xdb_oracle should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.oracle_dbinfo_.dbparam.user = tmp;

    tmp = xdb_oracle.child("pwd").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: pwd of the xdb_oracle should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.oracle_dbinfo_.dbparam.password = tmp;

    tmp = xdb_oracle.child("dbname").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: dbname of the xdb_oracle should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.oracle_dbinfo_.dbparam.database_name = tmp;

    tmp = xdb_oracle.child("charset").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: charset of the xdb_oracle should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.oracle_dbinfo_.dbparam.charset = tmp;
    params_.oracle_dbinfo_.dbparam.create_database_if_not_exists = false;
    params_.oracle_dbinfo_.dbparam.recreate_database_if_exists = false;

    //table config
    tmp = xtableconfig.child("product").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: the product_table_str_ should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.product_table_str_ = tmp;

    tmp = xtableconfig.child("instrument").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: the instrument_table_str_ should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.instrument_table_str_ = tmp;

    tmp = xtableconfig.child("calendar").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: the calendar_table_str_ should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.calendar_table_str_ = tmp;

    tmp = xtableconfig.child("insmrgnrate").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: the insmrgnrate_table_str_ should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.insmrgnrate_table_str_ = tmp;

    tmp = xtableconfig.child("dcepair_inscom").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: the dcepair_inscom_table_str_ should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.dcepair_inscom_table_str_ = tmp;

    //oracle table config
    tmp = xoracle_tableconfig.child("product").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: the product_table_str_ should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.oracle_product_table_str_ = tmp;

    tmp = xoracle_tableconfig.child("instrument").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: the instrument_table_str_ should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.oracle_instrument_table_str_ = tmp;

    tmp = xoracle_tableconfig.child("calendar").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: the calendar_table_str_ should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.oracle_calendar_table_str_ = tmp;

    tmp = xoracle_tableconfig.child("insmrgnrate").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: the insmrgnrate_table_str_ should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.oracle_insmrgnrate_table_str_ = tmp;

    tmp = xoracle_tableconfig.child("dcepair_inscom").text().as_string();
    if (tmp.empty()) {
        TRACE_ERROR(MODULE_NAME, DBSYNC_E_CONFIG_INVALID,
                "db_sync config: the dcepair_inscom_table_str_ should be specified");
        ASSIGN_AND_CHECK_LABEL(ret, DBSYNC_E_CONFIG_INVALID,
                end);
    }
    params_.oracle_dcepair_inscom_table_str_ = tmp;

    //xsync_time
    tmp = xsync_time.child("am_time").text().as_string();
    if (!tmp.empty()) {
        params_.sync_time_ = tmp;
    }

    LABEL_SCOPE_END;

end:
    return ret;
}

}
