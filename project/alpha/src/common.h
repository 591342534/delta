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
};

enum ATP_MESSAGE_TYPE
{
    ATP_MESSAGE_TYPE_INVALID = -1,
    ATP_MESSAGE_TYPE_QUERY_REQ = 1,
    ATP_MESSAGE_TYPE_TRADE_RSP = 2,
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

//expireinst_flag
const char EXPIREINST_FLAG_ALL = '0';
const char EXPIREINST_FLAG_SHFE = '1';
const char EXPIREINST_FLAG_CFFEX = '2';

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
}

#endif



