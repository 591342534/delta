/***************************************************************************** 
a50-zd Module Copyright (c) 2015. All Rights Reserved. 
FileName: product_business_deal.h
Version: 1.0 
Date: 2015.10.26 
History: cwm     2015.10.26   1.0     Create 
******************************************************************************/

#ifndef _PRODUCT_BUSINESS_DEAL_H_
#define _PRODUCT_BUSINESS_DEAL_H_

#include "utility/nocopyable.h"
#include "common.h"
#include "processor_base.h"
#include "base/dictionary.h"
#include "database/unidbpool.h"
#include <string>

using std::string;

namespace serverframe
{
// 订单状态 -1 失败 0 收单买单中 1 买单成功 2 卖单中3 卖单成功 4 买单中的订单中止中
enum ORDER_STATUS {
    ORDER_STATUS_FAILED = -1,                   /*失败*/
    ORDER_STATUS_RECEIVED_ON_WAY = 0,           /*收单中*/
    ORDER_STATUS_SUCCESS = 1,                   /*成功*/
    ORDER_STATUS_WITHDRAW_SUCCESS = 2,          /*withdraw成功*/
    ORDER_STATUS_ENTRUST_SUCCESS = 3,          /*entrust成功*/
};

// -1 非卖单 0 正常1 止盈 2 止损 3 到时中止5 强卖订单[上一天未正常中止]
enum SELL_STATUS {
    SELL_STATUS_NOT_PERMIT = -1, /*非卖单*/
    SELL_STATUS_NORMAL = 0, /*正常*/
    SELL_STATUS_STOP_PROFIT = 1, /*止盈*/
    SELL_STATUS_STOP_LOSS = 2, /*止损*/
    SELL_STATUS_TIME_END = 3, /*到时中止*/
    SELL_STATUS_FORCE_SELL = 5, /*强卖订单*/
};

//内部流程标志0 http_manager 1 policy_manager receive 2 policy_manager send 3 order receive 4 order send 5 deal received
enum SEND_STATUS {
    SEND_STATUS_HTTP_MANAGER_RECEIVED = 0,
    SEND_STATUS_POLICY_MANAGER_RECEIVED = 1,
    SEND_STATUS_POLICY_MANAGER_SEND = 2,
    SEND_STATUS_OERDER_RECEIVED = 3,
    SEND_STATUS_OERDER_SEND = 4,
    SEND_STATUS_DEAL_RECEIVED = 5,
};

//1: 订单已接收，并开始处理 2: 订单已委托 3: 撤单指令已接收，并开始处理 4: 订单有成交 5: 订单全部成交 6: 订单撤单成功（可能存在部分成交部分撤单）7: 订单失败

enum TRADE_ORDER_STATUS
{
    TRADE_ORDER_STATUS_NONE = 0,
    TRADE_ORDER_STATUS_ENTRUSTING = 1,        /* 订单已接收，并开始处理 */
    TRADE_ORDER_STATUS_ENTRUSTED = 2,         /* 订单已委托 */
    TRADE_ORDER_STATUS_WITHDRAWING = 3,       /* 撤单指令已接收，并开始处理 */
    TRADE_ORDER_STATUS_DEAL = 4,              /* 订单有成交 */
    TRADE_ORDER_STATUS_DEAL_ALL = 5,          /* 订单全部成交 */
    TRADE_ORDER_STATUS_WITHDRAWED = 6,        /* 订单撤单成功（可能存在部分成交部分撤单） */
    TRADE_ORDER_STATUS_FAILED = 7,            /* 订单失败 */
};

class product_business_deal 
    : public utility::nocopyable
{
public:
    static int check_deal_is_exist(base::dictionary &dict);
    static int get_stocker_attr(base::dictionary &d, const std::string &user_name);
    static int add_deal(base::dictionary &dict, database::db_instance* pconn = NULL);
};

}

#endif //_PRODUCT_BUSINESS_DEAL_H_
