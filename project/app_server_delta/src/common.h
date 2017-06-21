/*
 * common.h
 *
 *  Created on: 2017年4月13日
 *      Author: shengkaishan
 */

#ifndef __COMMON_H__
#define __COMMON_H__

#include <string>
namespace serverframe
{

/* manager    : 301000~301999 */
enum MANAGER_ERROR
{
    MANAGER_E_CONFIGFILE_INVALID = 301001,
    MANAGER_E_CONFIG_INVALID = 301002,
    MANAGER_E_PRODUCT_INVALID = 301003,
    MANAGER_E_DBCONNECT_INVALID = 301004,
    MANAGER_E_TRADE_DATE_INVALID = 301005,
    MANAGER_E_TRADE_TIME_INVALID = 301006,
    MANAGER_E_DBOPERATE_INVALID = 301007,
    MANAGER_E_ACCOUNT_INVALID = 301008,
    MANAGER_E_HTTP_INVALID = 301009,
    MANAGER_E_DB_FAILED = 301010,
    MANAGER_E_ORDER_TIMEOUT = 301011,
    MANAGER_E_ORDER_STATE_EXCEPTION = 301012,
};

const char *const MODULE_NAME = "manager_server";
const int MANAGER_S_OK = 0;

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

}

#endif /* TRUNK_MODULES_SIGMA_STOCKS_SOURCE_HTTP_MANAGER_SRC_COMMON_H_ */



