/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: common.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#ifndef __COMMON_H__
#define __COMMON_H__

#include "base/trace.h"
#include "base/dictionary.h"
#include <string>
namespace serverframe
{

/* manager    : 301000~301999 */
enum MANAGER_ERROR
{
    MANAGER_E_CONFIGFILE_INVALID = 301001,
    MANAGER_E_CONFIG_INVALID = 301002,
    MANAGER_E_DBCONNECT_INVALID = 301003,
    MANAGER_E_DB_FAILED = 301004,
    MANAGER_E_SERVER_NOTIFY_DATA_INVALID = 301005,
    MANAGER_E_FAILED = 301006,
};

enum ATP_MESSAGE_TYPE
{
    ATP_MESSAGE_TYPE_INVALID = -1,
    ATP_MESSAGE_TYPE_QUERY_REQ = 1,
    ATP_MESSAGE_TYPE_TRADE_RSP = 2,
    HTTP_CLIENT_MSG_TYPE_POST = 12,                 /* post a message */
    HTTP_CLIENT_MSG_TYPE_GET = 13,                  /* get a message */
};

/* atp message command defines */
const char* const ATPM_CMD_ERROR = "cmd_error";
const char* const ATPM_CMD_ENTRUST = "entrust";
const char* const ATPM_CMD_WITHDRAW = "withdraw";
const char* const ATPM_CMD_ENTRUST_RESULT = "entrust_result";
const char* const ATPM_CMD_DEAL_RESULT = "deal_result";
const char* const ATPM_CMD_WITHDRAW_RESULT = "withdraw_result";
const char* const ATPM_CMD_QRY_DEAL = "qry_deal";
const char* const ATPM_CMD_RSP_DEAL = "rsp_deal";
const char* const ATPM_CMD_QRY_ENTRUST = "qry_entrust";
const char* const ATPM_CMD_RSP_ENTRUST = "rsp_entrust";


const char *const MODULE_NAME = "alpha";
const int NAUT_S_OK = 0;

#define CHECK_IF_DBCONN_NULL(dbconn) \
if(dbconn == NULL){ \
    TRACE_ERROR(MODULE_NAME,MANAGER_E_DBCONNECT_INVALID, \
            "failed to get db conn "); \
    return MANAGER_E_DBCONNECT_INVALID;\
} \

#define DBCONN_TRAN_START(dbconn) \
if(!dbconn->_conn->begin_transaction()){ \
    TRACE_ERROR(MODULE_NAME,MANAGER_E_DB_FAILED, \
            "failed to start transaction"); \
    return MANAGER_E_DB_FAILED;\
} \

#define DBCONN_TRAN_COMMIT(dbconn) \
if(!dbconn->_conn->commit()){ \
    TRACE_ERROR(MODULE_NAME,MANAGER_E_DB_FAILED, \
            "failed to commit transaction"); \
    return MANAGER_E_DB_FAILED;\
} \

#define DBCONN_TRAN_ROLLBACK(dbconn) \
if(!dbconn->_conn->rollback()){ \
    TRACE_ERROR(MODULE_NAME,MANAGER_E_DB_FAILED, \
            "failed to rollback transaction"); \
    return MANAGER_E_DB_FAILED;\
} \

typedef base::ref_adapter<base::dictionary> ref_dictionary;

///////////////////////////////////////////////////////////////////////////////
#define MIN_COUNT_ACCOUNT			1
#define MIN_COUNT_DATE				10
#define MIN_COUNT_SQL				6
#define MAX_RECORD_COUNT			200
#define AVAIL_FLAG_FALSE			0

#define set_response_code(r) \
{ char buf[30]; sprintf(buf, "%d", r); evhttp_add_header(output_Headers, "code", buf); }

#define set_response_result(r) \
    evhttp_add_header(output_Headers, "result", r)

#define set_response_msg(r) \
    evhttp_add_header(output_Headers, "msg", r)

#define get_param(p1, p2, p3) \
    ((p1 == NULL) || (sscanf(p1, p2, p3) != 1))

#define PARAM_ERROR(nerrno)	\
{	\
    evbuffer_add_printf(evb, "%s", errno_transfer(nerrno));	\
    set_response_code(nerrno);	\
    set_response_result("N");	\
    set_response_msg(errno_transfer(nerrno));	\
    return MANAGER_E_FAILED;	\
}

#define SET_RESPONSE	\
    set_response_code(ret);	\
if (ret == NAUT_S_OK)	\
{	\
    evbuffer_add_printf(evb, "%s", base::djson::dict2str(outdict).c_str());	\
    set_response_result("Y");	\
    set_response_msg("success");	\
}	\
else \
{	\
    evbuffer_add_printf(evb, "%s", base::djson::dict2str(outdict).c_str());	\
    set_response_result("N");	\
    set_response_msg(errno_transfer(ret));	\
    return MANAGER_E_FAILED;	\
}

#define COPY_FIELD_STRING(field_name)	\
if (!(*ptr_dict)[field_name].is_null())	\
{	\
    indict.add_object(field_name, (*ptr_dict)[field_name].string_value());	\
}

#define ADD_FIELD_DB_STRING(dict,field_name)	\
    dict.add_object(field_name, dbconn->_conn->get_string(field_name))

#define ADD_FIELD_DB_LONG(dict,field_name)	\
    dict.add_object(field_name, dbconn->_conn->get_long(field_name))

#define ADD_FIELD_DB_DOUBLE(dict,field_name)	\
    dict.add_object(field_name, dbconn->_conn->get_double(field_name))

}

#endif



