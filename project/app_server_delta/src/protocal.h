#ifndef __PROTOCAL_H__
#define __PROTOCAL_H__

namespace serverframe
{
#pragma pack(push, 1)

///////////////////////////////////////////////////////////////////////////////
//instrument请求命令
const int TYPE_MANAGERSERVER_INSTRUMENT_REQ                 = 10001;
//instrument应答命令
const int TYPE_MANAGERSERVER_INSTRUMENT_RSP                 = 10002;


///////////////////////////////////////////////////////////////////////////////
const int TYPE_TRADE_GATEWAY_POSITION_RSP                    = 20000;
///////////////////////////////////////////////////////////////////////////////
const int TYPE_QUOTE_GATEWAY_MARKET_RSP                      = 30000;
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
    int request_id;
    int num;            //record number
}RSP_DATA_HEAD;

struct manager_server_instrument_req
{
    int UniqSequenceNo;
    char BrokerID[12];
    char InvestorID[14];
};

struct trade_gateway_position_field
{
    int UniqSequenceNo;
    char InstrumentID[32];
    char BrokerID[12];
    char InvestorID[14];
    char PosiDirection;
    char HedgeFlag;
    int YdPosition;
    int Position;
    int LongFrozen;
    int ShortFrozen;
    int CombPosition;;
    int CombLongFrozen;
    int CombShortFrozen;
    int TodayPosition;
    double MarginRateByMoney;
    double MarginRateByVolume;
};

#pragma pack(pop)
}
#endif
