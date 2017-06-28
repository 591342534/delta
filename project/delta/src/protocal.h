#ifndef __PROTOCAL_H__
#define __PROTOCAL_H__

#include <string.h>

namespace common
{
#pragma pack(push, 1)

///////////////////////////////////////////////////////////////////////////////
//instrument请求命令
const int TYPE_MANAGERSERVER_INSTRUMENT_REQ                  = 10001;
//instrument应答命令
const int TYPE_MANAGERSERVER_INSTRUMENT_RSP                  = 10002;
//insmrgnrate请求命令
const int TYPE_MANAGERSERVER_INSMRGNRATE_REQ                 = 10003;
//insmrgnrate应答命令
const int TYPE_MANAGERSERVER_INSMRGNRATE_RSP                 = 10004;
//dcepair请求命令
const int TYPE_MANAGERSERVER_DCEPAIR_REQ                     = 10005;
//dcepair应答命令
const int TYPE_MANAGERSERVER_DCEPAIR_RSP                     = 10006;
//expireinst请求命令
const int TYPE_MANAGERSERVER_EXPIREINST_REQ                  = 10007;
//expireinst应答命令
const int TYPE_MANAGERSERVER_EXPIREINST_RSP                  = 10008;

///////////////////////////////////////////////////////////////////////////////
const int TYPE_TRADE_GATEWAY_POSITION_RSP                    = 20000;
///////////////////////////////////////////////////////////////////////////////
const int TYPE_QUOTE_GATEWAY_MARKET_RSP                      = 30000;
///////////////////////////////////////////////////////////////////////////////
const int TYPE_CUSTOMER_CASH_DEPOSIT_RSP                      = 40000;
///////////////////////////////////////////////////////////////////////////////

typedef struct msg_header
{
    int type;
    int data_size;

    msg_header()
    :type(0),
     data_size(0)
    {}
}MSG_HEADER;

typedef struct rsp_data_head
{
    int status;         //0:success 1:failed
    int requestid;
    int num;            //record number
}RSP_DATA_HEAD;

struct manager_server_instrument_req
{
    int requestid;
    char BrokerID[12];
    char InvestorID[14];
};

struct manager_server_instrument_rsp
{
    char instrumentid[31];
    char exchangeid[9];
    char productid[31];
    char productclass;
    int volumemultiple;
    char optionstype;
    double strikeprice;
    int IsLast;
};

struct manager_server_insmrgnrate_req
{
    int requestid;
    char BrokerID[12];
    char InvestorID[14];
};

struct manager_server_insmrgnrate_rsp
{
    char tradingday[9];
    char contractcode[51];
    char exchangecode[9];
    char hedgeflag;
    char direction;
    double marginrate;
    double segmrgn;
    int IsLast;
};

struct manager_server_dcepair_req
{
    int requestid;
    char BrokerID[12];
    char InvestorID[14];
};

struct manager_server_dcepair_rsp
{
    char contractcode[51];
    char leftcont[51];
    char rightcont[51];
    char productcode[31];
    char leftprodcode[31];
    char rightprodcode[31];
    char productkind[31];
    char exchangecode[31];
    double permargin;
    char margindir;
    int mainseq;
    int groupseq;
    int contractseq;
    int groupid;
    int IsLast;
};

struct manager_server_expireinst_req
{
    int requestid;
    char BrokerID[12];
    char InvestorID[14];
};

struct manager_server_expireinst_rsp
{
    char instrumentid[31];
    char exchangeid[9];
    char productclass;
    char expiredate[9];
    char flag;
    int IsLast;
};

struct customer_cash_deposit
{
	char customer[15];
	double cffex;
	double czce;
	double dce;
	double shfe;
	double total;
	long time_stamps;
	customer_cash_deposit()
	:cffex(0.0)
	,czce(0.0)
	,dce(0.0)
	,shfe(0.0)
	,total(0)
	,time_stamps(0)
    {
        memset(customer, 0, sizeof(customer));
    }
};

#pragma pack(pop)
}
#endif
