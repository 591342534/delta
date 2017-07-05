/*****************************************************************************
 Nautilus Module ctp_quote Copyright (c) 2015. All Rights Reserved.

 FileName: ctp_quote.h
 Version: 1.0
 Date: 2015.9.30

 History:
 jeffguo     2015.9.30   1.0     Create
 ******************************************************************************/
#ifndef __CTP_QUOTE_H__
#define __CTP_QUOTE_H__

#include "ThostFtdcMdApi.h"
#include "common.h"
#include "base/base.h"
#include "base/sigslot.h"
#include <string>
#include <vector>

namespace ctp
{

struct ctp_mqserver_params
{
    std::string mq_name;
    std::string mq_host;
    int mq_port;
    std::string mq_topic;

    std::vector<std::string> sub_contracts;
    VBASE_HASH_MAP<std::string, bool> map_sub_fields;

    ctp_mqserver_params()
        : mq_name("mq")
        , mq_host("127.0.0.1")
        , mq_port(9600)
        , mq_topic("ctp")
    {
        map_sub_fields["mdate"] = true;
        map_sub_fields["mtime"] = true;
        map_sub_fields["code"] = true;
        map_sub_fields["excode"] = true;
        map_sub_fields["last"] = true;
        map_sub_fields["high"] = true;
        map_sub_fields["low"] = true;
        map_sub_fields["max"] = true;
        map_sub_fields["min"] = true;
        map_sub_fields["preclose"] = true;
        map_sub_fields["open"] = true;
        map_sub_fields["close"] = true;
        map_sub_fields["tvol"] = true;
        map_sub_fields["tval"] = true;
        map_sub_fields["preopeninter"] = true;
        map_sub_fields["openinter"] = true;
        map_sub_fields["buy1"] = true;
        map_sub_fields["bvol1"] = true;
        map_sub_fields["buy2"] = true;
        map_sub_fields["bvol2"] = true;
        map_sub_fields["buy3"] = true;
        map_sub_fields["bvol3"] = true;
        map_sub_fields["buy4"] = true;
        map_sub_fields["bvol4"] = true;
        map_sub_fields["buy5"] = true;
        map_sub_fields["bvol5"] = true;
        map_sub_fields["sell1"] = true;
        map_sub_fields["svol1"] = true;
        map_sub_fields["sell2"] = true;
        map_sub_fields["svol2"] = true;
        map_sub_fields["sell3"] = true;
        map_sub_fields["svol3"] = true;
        map_sub_fields["sell4"] = true;
        map_sub_fields["svol4"] = true;
        map_sub_fields["sell5"] = true;
        map_sub_fields["svol5"] = true;
    }
};

struct ctp_localize_params
{
    bool enable;
    bool compress;
    std::string root_dir;

    ctp_localize_params()
        : enable(false)
        , compress(false)
        , root_dir("")
    {}
};

struct ctp_quote_params
{
    std::string ctp_server;
    int ctp_port;
    std::string ctp_user;
    std::string ctp_password;
    std::string ctp_product_info;
    std::vector<std::string> ctp_contracts;
    ctp_localize_params localize_params;
    std::vector<ctp_mqserver_params> ar_mqserver_params;

    ctp_quote_params()
        : ctp_server("180.169.40.126")
        , ctp_port(21213)
        , ctp_user("11809117")
        , ctp_password("415966")
        , ctp_product_info("nautilus")
    {
    }
};

class ctp_quote
    : public CThostFtdcMdSpi
{
public:
    ctp_quote();
    virtual ~ctp_quote();

public:
    int start(const char* config_file);
    int stop();

protected:
    int load_config(const char* config_file, ctp_quote_params& params);
    int start_internal(int flag = 0);
    int stop_internal(int flag = 0);

public:
    const ctp_quote_params& params() const { return params_; }
    int get_request_id() { return request_id_++; }
    bool started() const { return started_; }

protected:
    int login();
    int logout();

    int subscribe_quote();
    int unsubscribe_quote();

private:
    int on_receive_quote_data(const CThostFtdcDepthMarketDataField *pDepthMarketData);
    std::string quote_json_string(const CThostFtdcDepthMarketDataField *pDepthMarketData,
            VBASE_HASH_MAP<std::string, bool>& map_sub_fields);
    char* change_date_format(char* outdate, char* inputdate);
    std::string gbk_to_utf8(const char* text);

protected:
    virtual void OnFrontConnected();
    virtual void OnFrontDisconnected(int nReason);
    virtual void OnHeartBeatWarning(int nTimeLapse);
    virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);
    virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);
    virtual void OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp);

private:
    ctp_quote_params params_;
    CThostFtdcMdApi* ctp_;
    int request_id_;
    bool started_;
};

}

#endif
