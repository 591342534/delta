/*****************************************************************************
Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

FileName: trade_struct.h
Version: 1.0
Date: 2017.08.08

History:
eric     2017.08.08   1.0     Create
******************************************************************************/

#ifndef __NAUT_CTPTRADE_TRADE_STRUCT_H__
#define __NAUT_CTPTRADE_TRADE_STRUCT_H__

#include "base/reference_base.h"
#include "base/dictionary.h"

namespace ctp
{

enum ATP_MESSAGE_TYPE
{
	ATP_MESSAGE_TYPE_INVALID = -1,

	ATP_MESSAGE_TYPE_IN_ORDER = 1,						/* receive an order */
	ATP_MESSAGE_TYPE_DEAL_PUSH = 2,						/* push a deal result */
	ATP_MESSAGE_TYPE_ORDER_REPLY = 3,					/* push reply for order */

	ATP_MESSAGE_TYPE_IN_QUERY = 4,						/* receive a query */
	ATP_MESSAGE_TYPE_QUERY_REPLY = 5,					/* push reply for query */

	ATP_MESSAGE_TYPE_SERVER_TRADE_REQ = 6,				/* trade(entrust/cancel) request */
	ATP_MESSAGE_TYPE_SERVER_TRADE_RSP = 7,				/* response for a trade request */
	ATP_MESSAGE_TYPE_SERVER_QUERY_REQ = 8,				/* query(account/deal/entrust) request */
	ATP_MESSAGE_TYPE_SERVER_QUERY_RSP = 9,				/* response for a query request */

	ATP_MESSAGE_TYPE_CHECKER_REQ = 10,					/* pull deal/entrust info by order checker */
	ATP_MESSAGE_TYPE_CHECKER_RSP = 11,					/* response for a deal/entrust pull request*/

	ATP_MESSAEG_TYPE_SERVER_STATUS = 12,				/* the state of the trader server */
	ATP_MESSAGE_TYPE_IN_SPECIAL_QUERY = 13,
};

enum ORDER_STATUS
{
	ORDER_STATUS_NONE = 0,
	ORDER_STATUS_ENTRUSTING = 1,		/* 订单已接收，并开始处理 */
	ORDER_STATUS_ENTRUSTED = 2,			/* 订单已委托 */
	ORDER_STATUS_WITHDRAWING = 3,		/* 撤单指令已接收，并开始处理 */
	ORDER_STATUS_DEAL = 4,				/* 订单有成交 */
	ORDER_STATUS_DEAL_ALL = 5,			/* 订单全部成交 */
	ORDER_STATUS_WITHDRAWED = 6,		/* 订单撤单成功（可能存在部分成交部分撤单） */
	ORDER_STATUS_FAILED = 7,			/* 订单失败 */
};

#pragma pack(push, 1)

struct atp_message
{
	int type;
	void* param1;
	void* param2;
};

#pragma pack(pop)

class message_dispatcher
{
public:
	virtual ~message_dispatcher() {}

public:
	virtual int dispatch_message(atp_message& msg) = 0;
};

typedef base::ref_adapter<base::dictionary> ref_dictionary;

}

#endif  //__NAUT_CTPTRADE_TRADE_STRUCT_H__