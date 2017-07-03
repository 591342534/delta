/*****************************************************************************
 data_query_manager Copyright (c) 2016. All Rights Reserved.

 FileName: product_business_deal.cpp
 Version: 1.0
 Date: 2016.11.1

 History:
 ericsheng     2016.11.1   1.0     Create
 ******************************************************************************/

#include "product_business_deal.h"
#include "process.h"
#include "base/trace.h"
#include "database/dbscope.h"
#include "project_server.h"
#include "common.h"
namespace serverframe
{

product_business_deal::product_business_deal()
{
    
}

product_business_deal::~product_business_deal()
{
    
}

std::string product_business_deal::make_sqlcondition_prefix(std::string &current)
{
    if (current.empty())
    {
        current = " where ";
    }
    else
    {
        current += " and ";
    }
    return current;
}

std::string product_business_deal::make_condition_dict_str(base::dictionary &dict, const std::string &fieldname, const std::string &tablename)
{
    if (tablename.empty())
    {
        return fieldname + "='" + dict[fieldname].string_value() + "'";
    }
    else
    {
        return tablename + "." + fieldname + "='" + dict[fieldname].string_value() + "'";
    }
}

std::string product_business_deal::make_sqlcondition(std::string &current, const std::string &append)
{
    make_sqlcondition_prefix(current);
    current += append;
    return current;
}

std::string product_business_deal::make_sqlcondition(std::string &current, base::dictionary &dict, const std::string &fieldname, const std::string &tablename)
{
    if (!dict[fieldname].is_null())
    {
        make_sqlcondition_prefix(current);
        current += make_condition_dict_str(dict, fieldname, tablename);
    }
    return current;
}

std::string product_business_deal::make_sql_date(std::string &current, base::dictionary &dict, const std::string &fieldname, const std::string &tablename)
{
    make_sql_from_date(current, dict, fieldname, tablename);
    make_sql_end_date(current, dict, fieldname, tablename);
    return current;
}

std::string product_business_deal::make_sql_from_date(std::string &current, base::dictionary &dict, const std::string &fieldname, const std::string &tablename)
{
    if (!dict["tmp"].is_null())
    {
        make_sqlcondition_prefix(current);
        if (tablename.empty())
        {
            current += fieldname + " >='" + dict["tmp"].string_value() + "'";
        }
        else
        {
            current += tablename + "." + fieldname + " >='" + dict["tmp"].string_value() + "'";
        }
    }
    return current;
}

std::string product_business_deal::make_sql_end_date(std::string &current, base::dictionary &dict, const std::string &fieldname, const std::string &tablename)
{
    if (!dict["tmp"].is_null())
    {
        make_sqlcondition_prefix(current);
        if (tablename.empty())
        {
            current += fieldname + " <='" + dict["tmp"].string_value() + "'";
        }
        else
        {
            current += tablename + "." + fieldname + " <='" + dict["tmp"].string_value() + "'";
        }
    }
    return current;
}

std::string product_business_deal::make_sql_startindex(std::string &current, base::dictionary &dict, const std::string &tablename)
{
    int start_index = dict["tmp"].int_value();

    if (start_index > 0)
    {
        if (dict["tmp"].int_value() == 1)
        {
            if (tablename.empty())
            {
                current += " and id<" + comm::itos(start_index);
            }
            else
            {
                current += " and " + tablename + ".id<" + comm::itos(start_index);
            }
        }
        else
        {
            if (tablename.empty())
            {
                current += " and id>=" + comm::itos(start_index);
            }
            else
            {
                current += " and " + tablename + ".id>=" + comm::itos(start_index);
            }
        }
    }

    return current;
}

std::string product_business_deal::make_sql_limit(base::dictionary &dict)
{
    std::string limit;
    int recordnum = dict["tmp"].int_value();
    if (recordnum <= 0 || recordnum > MAX_RECORD_COUNT)
    {
        limit = " limit " + comm::itos(MAX_RECORD_COUNT);
    }
    else
    {
        limit = " limit " + dict["tmp"].string_value();
    }
    return limit;
}

void product_business_deal::make_condition_field(sqlmaker &sql, const base::dictionary &dict, const std::string &fieldname, sqlmaker::datatype type)
{
    if (!dict[fieldname].is_null())
    {
        if (sqlmaker::dtstring == type)
        {
            sql.condition_field(fieldname, dict[fieldname].string_value());
        }
        else if (sqlmaker::dtint == type)
        {
            sql.condition_field(fieldname, dict[fieldname].int_value());
        }
        else if (sqlmaker::dtdouble == type)
        {
            sql.condition_field(fieldname, dict[fieldname].double_value());
        }
    }
}

void product_business_deal::make_update_field(sqlmaker &sql, const base::dictionary &dict, const std::string &fieldname, sqlmaker::datatype type)
{
    if (!dict[fieldname].is_null())
    {
        if (sqlmaker::dtstring == type)
        {
            sql.update_field(fieldname, dict[fieldname].string_value());
        }
        else if (sqlmaker::dtint == type)
        {
            sql.update_field(fieldname, dict[fieldname].int_value());
        }
        else if (sqlmaker::dtdouble == type)
        {
            sql.update_field(fieldname, dict[fieldname].double_value());
        }
    }
}

void product_business_deal::set_sql_error_response(base::dictionary &dict, int nerrno, const std::string &error)
{
    dict["errno"] = nerrno;
    dict["error"] = error;
}

bool product_business_deal::query_sql_check(const std::string &sql)
{
    std::string s = sql;
    comm::string_replace(s, " ", "");
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s.substr(0, 6) != "select")
    {
        return false;
    }
    if (s.find("forupdate") != std::string::npos)
    {
        return false;
    }

    return true;
}

//查询
int product_business_deal::sql_query(base::dictionary& indict, base::dictionary& outdict)
{
    database::dbscope mysql_db_keepper(
        get_project_server().get_db_conn_pool());
    database::db_instance* dbconn = mysql_db_keepper.get_db_conn();

    std::string sql = indict["tmp"].string_value();
    if (!query_sql_check(sql))
    {
        TRACE_WARNING(MODULE_NAME, "%s:sql illegal:%s", __FUNCTION__, sql.c_str());
        set_sql_error_response(outdict, MANAGER_E_FAILED, "sql illegal");
        return MANAGER_E_FAILED;
    }

    base::darray da;

    if(dbconn->_conn->query(sql.c_str()))
    {
        std::vector<std::string> vfields = dbconn->_conn->get_fields();
        while(dbconn->_conn->fetch_row())
        {
            base::dictionary d;
            for (std::vector<std::string>::iterator it = vfields.begin(); it != vfields.end(); it++)
            {
                d.add_object(*it, dbconn->_conn->get_string(*it));
            }
            da.append(d);
        }
    }
    else
    {
        TRACE_ERROR(MODULE_NAME, MANAGER_E_FAILED, "%s:db error[%d][%s]:%s", __FUNCTION__,
            dbconn->_conn->get_errno(), dbconn->_conn->get_error().c_str(), sql.c_str());
        set_sql_error_response(outdict, dbconn->_conn->get_errno_inner(), dbconn->_conn->get_error_inner());
        return MANAGER_E_FAILED;
    }

    outdict.add_object("tmp", da);
    outdict.add_object("two", (int)da.count());
    outdict.add_object("three", base::util::local_date_time_string());
    return 0;
}

int product_business_deal::account_fund_query(base::dictionary& indict, base::dictionary& outdict)
{
    database::dbscope mysql_db_keepper(
        get_project_server().get_db_conn_pool());
    database::db_instance* dbconn = mysql_db_keepper.get_db_conn();

    std::string condition;
    make_sqlcondition(condition, indict, "productid");
    make_sqlcondition(condition, indict, "productclass");

    char sql[1024] = { 0 };
    sprintf(sql, "select * from %s %s", get_project_server().get_process_param().instrument_table_str_.c_str(), condition.c_str());

    base::darray da;

    if(dbconn->_conn->query(sql))
    {
        while (dbconn->_conn->fetch_row())
        {
            base::dictionary d, out;
            ADD_FIELD_DB_STRING(d, "instrumentid");
            ADD_FIELD_DB_STRING(d, "instrumentname");
            ADD_FIELD_DB_LONG(d, "volumemultiple");
            da.append(d);
        }
    }
    else
    {
        TRACE_ERROR(MODULE_NAME, MANAGER_E_FAILED, "%s:db error[%d][%s]:%s", __FUNCTION__,
            dbconn->_conn->get_errno(), dbconn->_conn->get_error().c_str(), sql);
        set_sql_error_response(outdict, dbconn->_conn->get_errno_inner(), dbconn->_conn->get_error_inner());
        return MANAGER_E_FAILED;
    }

    outdict.add_object("data", da);
    outdict.add_object("count", (int)da.count());
    outdict.add_object("current_time", base::util::local_date_time_string());
    return 0;
}

int product_business_deal::trade_system_config_insert(base::dictionary& indict, base::dictionary& outdict)
{
    database::dbscope mysql_db_keepper(
        get_project_server().get_db_conn_pool());
    database::db_instance* dbconn = mysql_db_keepper.get_db_conn();

    sqlmaker sql(get_project_server().get_process_param().instrument_table_str_, sqlmaker::mtinsert);
    sql.add_field("tmp", indict["tmp"].string_value());
    sql.add_field("tmp", indict["tmp"].int_value());
    sql.add_field("tmp", indict["tmp"].int_value());

    if (!dbconn->_conn->execute(sql.sql().c_str()))
    {
        TRACE_ERROR(MODULE_NAME, MANAGER_E_FAILED, "%s:db error[%d][%s]:%s", __FUNCTION__,
            dbconn->_conn->get_errno(), dbconn->_conn->get_error().c_str(), sql.sql().c_str());
        set_sql_error_response(outdict, dbconn->_conn->get_errno_inner(), dbconn->_conn->get_error_inner());
        return MANAGER_E_FAILED;
    }

    return 0;
}

int product_business_deal::trade_system_config_update(base::dictionary& indict, base::dictionary& outdict)
{
    database::dbscope mysql_db_keepper(
        get_project_server().get_db_conn_pool());
    database::db_instance* dbconn = mysql_db_keepper.get_db_conn();

    sqlmaker sql(get_project_server().get_process_param().instrument_table_str_, sqlmaker::mtupdate);
    make_update_field(sql, indict, "tmp", sqlmaker::dtint);
    make_update_field(sql, indict, "tmp", sqlmaker::dtint);
    make_condition_field(sql, indict, "tmp", sqlmaker::dtstring);

    if (!dbconn->_conn->execute(sql.sql().c_str()))
    {
        TRACE_ERROR(MODULE_NAME, MANAGER_E_FAILED, "%s:db error[%d][%s]:%s", __FUNCTION__,
            dbconn->_conn->get_errno(), dbconn->_conn->get_error().c_str(), sql.sql().c_str());
        set_sql_error_response(outdict, dbconn->_conn->get_errno_inner(), dbconn->_conn->get_error_inner());
        return MANAGER_E_FAILED;
    }

    if (0 == dbconn->_conn->affected_rows())
    {
        TRACE_WARNING(MODULE_NAME, "%s:db error, affected 0 rows: %s", __FUNCTION__, sql.sql().c_str());
        return MANAGER_E_FAILED;
    }

    return 0;
}

int product_business_deal::trade_system_config_delete(base::dictionary& indict, base::dictionary& outdict)
{
    database::dbscope mysql_db_keepper(
        get_project_server().get_db_conn_pool());
    database::db_instance* dbconn = mysql_db_keepper.get_db_conn();

    sqlmaker sql(get_project_server().get_process_param().instrument_table_str_, sqlmaker::mtdelete);
    make_condition_field(sql, indict, "tmp", sqlmaker::dtstring);

    if (!dbconn->_conn->execute(sql.sql().c_str()))
    {
        TRACE_ERROR(MODULE_NAME, MANAGER_E_FAILED, "%s:db error[%d][%s]:%s", __FUNCTION__,
            dbconn->_conn->get_errno(), dbconn->_conn->get_error().c_str(), sql.sql().c_str());
        set_sql_error_response(outdict, dbconn->_conn->get_errno_inner(), dbconn->_conn->get_error_inner());
        return MANAGER_E_FAILED;
    }

    if (0 == dbconn->_conn->affected_rows())
    {
        TRACE_WARNING(MODULE_NAME, "%s:db error, affected 0 rows: %s", __FUNCTION__, sql.sql().c_str());
        return MANAGER_E_FAILED;
    }

    return 0;
}


}
