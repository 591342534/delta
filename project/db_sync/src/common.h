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

#include <string>
#include "libiconv/iconv.h"
namespace db_sync
{

/* manager    : 301000~301999 */
enum MANAGER_ERROR
{
    DBSYNC_E_CONFIGFILE_INVALID = 301001,
    DBSYNC_E_CONFIG_INVALID = 301002,
    DBSYNC_E_PRODUCT_INVALID = 301003,
    DBSYNC_E_DBCONNECT_INVALID = 301004,
    DBSYNC_E_TRADE_DATE_INVALID = 301005,
    DBSYNC_E_TRADE_TIME_INVALID = 301006,
    DBSYNC_E_DBOPERATE_INVALID = 301007,
    DBSYNC_E_ACCOUNT_INVALID = 301008,
    DBSYNC_E_HTTP_INVALID = 301009,
    DBSYNC_E_DB_FAILED = 301010,
    DBSYNC_E_ORDER_TIMEOUT = 301011,
    DBSYNC_E_ORDER_STATE_EXCEPTION = 301012,
};

const char *const MODULE_NAME = "db_sync";
const int DB_SYNC_S_OK = 0;

#define CHECK_IF_DBCONN_NULL(dbconn) \
if(dbconn == NULL){ \
    TRACE_ERROR(MODULE_NAME,DBSYNC_E_DBCONNECT_INVALID, \
            "failed to get db conn "); \
    return DBSYNC_E_DBCONNECT_INVALID;\
} \

#define DBCONN_TRAN_START(dbconn) \
if(!dbconn->_conn->begin_transaction()){ \
    TRACE_ERROR(MODULE_NAME,DBSYNC_E_DB_FAILED, \
            "failed to start transaction"); \
    return DBSYNC_E_DB_FAILED;\
} \

#define DBCONN_TRAN_COMMIT(dbconn) \
if(!dbconn->_conn->commit()){ \
    TRACE_ERROR(MODULE_NAME,DBSYNC_E_DB_FAILED, \
            "failed to commit transaction"); \
    return DBSYNC_E_DB_FAILED;\
} \

#define DBCONN_TRAN_ROLLBACK(dbconn) \
if(!dbconn->_conn->rollback()){ \
    TRACE_ERROR(MODULE_NAME,DBSYNC_E_DB_FAILED, \
            "failed to rollback transaction"); \
    return DBSYNC_E_DB_FAILED;\
} \

inline std::string gbk_to_utf8(const char* text)
{
    iconv_t ih = iconv_open("utf-8", "gbk");

    size_t in_length = strlen(text);
    size_t out_length = 2 * in_length;
    char* in_buffer = new char[in_length + 1];
    char* out_buffer = new char[out_length + 1];
    char* in_buffer2 = in_buffer;
    char* out_buffer2 = out_buffer;
    out_buffer[0] = 0x00;

    strcpy(in_buffer, text);
    iconv(ih, &in_buffer, &in_length, &out_buffer, &out_length);
    *out_buffer = 0x00;
    std::string ret = out_buffer2;

    delete[]in_buffer2;
    delete[]out_buffer2;

    iconv_close(ih);

    return ret;
}
}

#endif



