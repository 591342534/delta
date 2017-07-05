/*****************************************************************************
 Nautilus Module ctp_quote Copyright (c) 2015. All Rights Reserved.

 FileName: ctp_quote.cpp
 Version: 1.0
 Date: 2015.9.30

 History:
 jeffguo     2015.9.30   1.0     Create
 ******************************************************************************/

#include "ctp_quote.h"
#include "base/base.h"
#include "base/trace.h"
#include "base/pugixml.hpp"
#include "base/dispatch.h"
#include "base/file.h"
#include "common.h"
#include <stdio.h>
#include <string>
#include <float.h>
#include "libiconv/iconv.h"

#define CTP_DOUBLE_LIMIT 99999999999999
#define CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, val) do { \
		dst = val > 0 ? val : -val; \
		if (dst > CTP_DOUBLE_LIMIT) { \
			dst = 0.0f; \
		} \
	} while(0);

namespace ctp
{

ctp_quote::ctp_quote()
	: ctp_(NULL)
	, request_id_(0)
	, started_(false)
{

}

ctp_quote::~ctp_quote()
{
	stop();
}

int ctp_quote::start(const char* config_file)
{
	if (started_) {
		return NAUT_S_OK;
	}

	int ret = NAUT_S_OK;

	ret = load_config(config_file, params_);
	CHECK_LABEL(ret, end);

	ret = start_internal();

end:
	if (BSUCCEEDED(ret)) {
		started_ = true;
	}
	else {
		stop();
	}
	return ret;
}

int ctp_quote::stop()
{
	started_ = false;
	return stop_internal();
}

int ctp_quote::start_internal(int flag)
{
	stop_internal(flag);

	int ret = NAUT_S_OK;

	LABEL_SCOPE_START;

	/* initialize ctp server */
	char addr[256];
	sprintf(addr, "tcp://%s:%d", params_.ctp_server.c_str(), params_.ctp_port);
    base::file::make_sure_directory_exist("./ctp-log");
    ctp_ = CThostFtdcMdApi::CreateFtdcMdApi("./ctp-log");
	ctp_->RegisterFront(addr);
	ctp_->RegisterSpi(this);
	ctp_->Init();

	LABEL_SCOPE_END;

end:
	return ret;
}

int ctp_quote::stop_internal(int flag)
{
	if (ctp_ != NULL) {
		if (flag == 0) {
			//unsubscribe_quote();
			logout();
		}

		ctp_->RegisterSpi(NULL);
		ctp_->Release();
		ctp_ = NULL;
	}

	return NAUT_S_OK;
}

int ctp_quote::load_config(const char* config_file, ctp_quote_params& params)
{
	int ret = NAUT_S_OK;

	LABEL_SCOPE_START;
    std::string tmp;
	pugi::xml_document doc;
	if (!doc.load_file(config_file)) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIGFILE_INVALID,
				"ctp-quote config file is not exist or invalid '%s'", config_file);
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIGFILE_INVALID, end);
	}

    /* config of ctp quote */
	pugi::xml_node xctp = doc.child("ctp-quote").child("ctp");
	if (xctp.empty()) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIG_INVALID,
				"ctp-quote config: element 'ctp' is not exist");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIG_INVALID, end);
	}

	tmp = xctp.child("host").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIG_INVALID,
				"ctp-quote config: host of the ctp-server should be specified");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIG_INVALID, end);
	}
	params.ctp_server = tmp;

	tmp = xctp.child("port").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIG_INVALID,
				"ctp-quote config: port of the ctp-server should be specified");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIG_INVALID, end);
	}
	params.ctp_port = atoi(tmp.c_str());

	tmp = xctp.child("user").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIG_INVALID,
				"ctp-quote config: the user name of the ctp-server should be specified");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIG_INVALID, end);
	}
	params.ctp_user = tmp;

	tmp = xctp.child("password").text().as_string();
	if (tmp.empty()) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIG_INVALID,
				"ctp-quote config: the password of the ctp-server user should be specified");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIG_INVALID, end);
	}
	params.ctp_password = tmp;

	tmp = xctp.child("product-info").text().as_string();
	if (!tmp.empty()) {
		params.ctp_product_info = tmp;
	}

	/* parse ctp subscribe contracts */
	pugi::xml_node xcontracts = xctp.child("sub-contracts");
	if (xcontracts.empty()) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIG_INVALID,
				"ctp-quote config: element 'sub-contracts' of ctp is not exist");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIG_INVALID, end);
	}

	pugi::xml_node xcontract = xcontracts.child("contract");
	while (!xcontract.empty()) {
		tmp = xcontract.text().as_string();
		if (tmp.empty()) {
			TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIG_INVALID,
					"ctp-quote config: contract value seems empty.");
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIG_INVALID, end);
		}
		params.ctp_contracts.push_back(tmp);
		xcontract = xcontract.next_sibling("contract");
	}

	/* localization params */
	pugi::xml_node xlocalization = doc.child("ctp-quote").child("localization");
	tmp = xlocalization.attribute("enable").as_string();
	if (tmp == "true") {
		params.localize_params.enable = true;
	}
	else {
		params.localize_params.enable = false;
	}

	if (params.localize_params.enable) {
		tmp = xlocalization.child("compress").text().as_string();
		if (tmp == "true") {
			params.localize_params.compress = true;
		}
		else {
			params.localize_params.compress = false;
		}

		tmp = xlocalization.child("root-dir").text().as_string();
		if (!tmp.empty()) {
			params.localize_params.root_dir = tmp;
		}
	}

	/* config of mqserver */
	pugi::xml_node xmqserver = doc.child("ctp-quote").child("mqserver");
	if (xmqserver.empty()) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIG_INVALID,
				"ctp-quote config: element 'mqserver' is not exist");
		ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIG_INVALID, end);
	}

	while (!xmqserver.empty())
	{
		if (strcmp(xmqserver.attribute("enable").as_string(), "true") != 0) {
			xmqserver = xmqserver.next_sibling("mqserver");
			continue;
		}

		ctp_mqserver_params mqserver_params;

		tmp = xmqserver.child("name").text().as_string();
		if (tmp.empty()) {
			TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIG_INVALID,
					"ctp-quote config: name of the mqserver should be specified");
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIG_INVALID, end);
		}
		mqserver_params.mq_name = tmp;

		tmp = xmqserver.child("host").text().as_string();
		if (tmp.empty()) {
			TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIG_INVALID,
					"ctp-quote config: host of the mqserver should be specified");
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIG_INVALID, end);
		}
		mqserver_params.mq_host = tmp;

		tmp = xmqserver.child("port").text().as_string();
		if (tmp.empty()) {
			TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIG_INVALID,
					"ctp-quote config: port of the mqserver should be specified");
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIG_INVALID, end);
		}
		mqserver_params.mq_port = atoi(tmp.c_str());

		tmp = xmqserver.child("topic").text().as_string();
		if (!tmp.empty()) {
			mqserver_params.mq_topic = tmp;
		}

		/* parse mqserver subscribe contracts */
		pugi::xml_node xcontracts = xmqserver.child("sub-contracts");
		if (xcontracts.empty()) {
			TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIG_INVALID,
					"ctp-quote config: element 'sub-contracts' of mqserver '%s' is not exist",
					mqserver_params.mq_name.c_str());
			ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIG_INVALID, end);
		}

		pugi::xml_node xcontract = xcontracts.child("contract");
		while (!xcontract.empty()) {
			tmp = xcontract.text().as_string();
			if (tmp.empty()) {
				TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIG_INVALID,
						"ctp-quote config: contract value seems empty.");
				ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIG_INVALID, end);
			}
			mqserver_params.sub_contracts.push_back(tmp);
			xcontract = xcontract.next_sibling("contract");
		}

		/* parse subscribe fields */
		pugi::xml_node xfields = xmqserver.child("sub-fields");
		if (!xfields.empty()) {
			pugi::xml_node xfield = xfields.child("field");
			while (!xfield.empty()) {
				tmp = xfield.text().as_string();
				if (tmp.empty()) {
					TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_CONFIG_INVALID,
							"ctp-quote config: field value seems empty.");
					ASSIGN_AND_CHECK_LABEL(ret, NAUT_CTPQUOTE_E_CONFIG_INVALID, end);
				}
				if (strcmp(xfield.attribute("enable").as_string(), "true") == 0) {
					mqserver_params.map_sub_fields[tmp] = true;
				}
				else {
					mqserver_params.map_sub_fields[tmp] = false;
				}
				xfield = xfield.next_sibling("field");
			}
		}

		params.ar_mqserver_params.push_back(mqserver_params);
		xmqserver = xmqserver.next_sibling("mqserver");
	}

	LABEL_SCOPE_END;

end:
	return ret;
}

int ctp_quote::login()
{
	assert(ctp_ != NULL);

	CThostFtdcReqUserLoginField req;
	memset(&req, 0, sizeof(req));

	strcpy(req.UserID, params_.ctp_user.c_str());
	strcpy(req.Password, params_.ctp_password.c_str());
	strcpy(req.ProtocolInfo, params_.ctp_product_info.c_str());

	int ret = ctp_->ReqUserLogin(&req, get_request_id());
	if (ret != 0) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_LOGIN_FAILED,
				"ctp quote login failed, ret: %d", ret);
		return NAUT_CTPQUOTE_E_LOGIN_FAILED;
	}

	TRACE_INFO("ctp-quote", "login successfully");
	return NAUT_S_OK;
}

int ctp_quote::logout()
{
	if (ctp_ == NULL) {
		return NAUT_S_OK;
	}

	CThostFtdcUserLogoutField req;
	memset(&req, 0, sizeof(req));

	strcpy(req.UserID, params_.ctp_user.c_str());
	int ret = ctp_->ReqUserLogout(&req, get_request_id());
	if (ret != 0) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_LOGOUT_FAILED,
				"ctp quote logout failed, ret: %d", ret);
		return NAUT_CTPQUOTE_E_LOGOUT_FAILED;
	}

	return NAUT_S_OK;
}

int ctp_quote::subscribe_quote()
{
	assert(ctp_ != NULL);

	char buffer[256];
	char* contract = &buffer[0];
	for (int i = 0; i < (int)params_.ctp_contracts.size(); i++) {
		strcpy(contract, params_.ctp_contracts[i].c_str());
		int ret = ctp_->SubscribeMarketData(&contract, 1);
		if (ret != 0) {
			TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_SUBMARKETDATA_FAILED,
					"subscribe market data failed, contract: '%s', ret: %d",
					contract, ret);
		}
	}
	return NAUT_S_OK;
}

int ctp_quote::unsubscribe_quote()
{
	assert(ctp_ != NULL);

	char buffer[256];
	char* contract = &buffer[0];
	for (int i = 0; i < (int)params_.ctp_contracts.size(); i++) {
		strcpy(contract, params_.ctp_contracts[i].c_str());
		int ret = ctp_->UnSubscribeMarketData(&contract, 1);
		if (ret != 0) {
			TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_UNSUBMARKETDATA_FAILED,
					"unsubscribe market data failed, contract: '%s', ret: %d",
					contract, ret);
		}
	}
	return NAUT_S_OK;
}

int ctp_quote::on_receive_quote_data(const CThostFtdcDepthMarketDataField *pDepthMarketData)
{
    std::string quote_string = quote_json_string(pDepthMarketData,
        params_.ar_mqserver_params[0].map_sub_fields);
    TRACE_INFO("ctp-quote", "publish ctp quote: %s", quote_string.c_str());

	return NAUT_S_OK;
}

void print_double(double x)
{
	char* t = (char*)(double*)&x;
	for (int i = 0; i < 8; i++) {
		printf("%02x-", (*t));
		t++;
	}
	printf("\n");
}

std::string ctp_quote::quote_json_string(const CThostFtdcDepthMarketDataField *pDepthMarketData,
		VBASE_HASH_MAP<std::string, bool>& map_sub_fields)
{
	const int max_quote_string_length = 8192;
	char json_string[max_quote_string_length];
	char tmp[1024];
	double dst = 0.0;

	sprintf(json_string, "{");

	if (map_sub_fields["mdate"]) {
        char datebuf[11];
        memset(datebuf, 0, sizeof(datebuf));
        change_date_format(datebuf, (char*)pDepthMarketData->ActionDay);
		sprintf(tmp, "\"mdate\":\"%s\",", datebuf);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["mtime"]) {
		sprintf(tmp, "\"mtime\":\"%s\",", pDepthMarketData->UpdateTime);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["code"]) {
		sprintf(tmp, "\"code\":\"%s\",", pDepthMarketData->InstrumentID);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["excode"]) {
	    if(strlen(pDepthMarketData->ExchangeID) != 0) {
	        sprintf(tmp, "\"excode\":\"%s\",", pDepthMarketData->ExchangeID);
	    } else {
	        sprintf(tmp, "\"excode\":\"%s\",", "SHFE");
	    }
		strcat(json_string, tmp);
	}

	if (map_sub_fields["last"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->LastPrice);
		sprintf(tmp, "\"last\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["high"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->HighestPrice);
		sprintf(tmp, "\"high\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["low"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->LowestPrice);
		sprintf(tmp, "\"low\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["max"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->UpperLimitPrice);
		sprintf(tmp, "\"max\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["min"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->LowerLimitPrice);
		sprintf(tmp, "\"min\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["preclose"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->PreClosePrice);
		sprintf(tmp, "\"preclose\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["open"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->OpenPrice);
		sprintf(tmp, "\"open\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["close"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->ClosePrice);
		sprintf(tmp, "\"close\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["tvol"]) {
		sprintf(tmp, "\"tvol\":%d,", pDepthMarketData->Volume);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["tval"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->Turnover);
		sprintf(tmp, "\"tval\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["openinter"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->OpenInterest);
		sprintf(tmp, "\"openinter\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["preopeninter"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->PreOpenInterest);
		sprintf(tmp, "\"preopeninter\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["buy1"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->BidPrice1);
		sprintf(tmp, "\"buy1\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["bvol1"]) {
		sprintf(tmp, "\"bvol1\":%d,", pDepthMarketData->BidVolume1);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["buy2"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->BidPrice2);
		sprintf(tmp, "\"buy2\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["bvol2"]) {
		sprintf(tmp, "\"bvol2\":%d,", pDepthMarketData->BidVolume2);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["buy3"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->BidPrice3);
		sprintf(tmp, "\"buy3\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["bvol3"]) {
		sprintf(tmp, "\"bvol3\":%d,", pDepthMarketData->BidVolume3);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["buy4"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->BidPrice4);
		sprintf(tmp, "\"buy4\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["bvol4"]) {
		sprintf(tmp, "\"bvol4\":%d,", pDepthMarketData->BidVolume4);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["buy5"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->BidPrice5);
		sprintf(tmp, "\"buy5\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["bvol5"]) {
		sprintf(tmp, "\"bvol5\":%d,", pDepthMarketData->BidVolume5);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["sell1"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->AskPrice1);
		sprintf(tmp, "\"sell1\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["svol1"]) {
		sprintf(tmp, "\"svol1\":%d,", pDepthMarketData->AskVolume1);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["sell2"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->AskPrice2);
		sprintf(tmp, "\"sell2\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["svol2"]) {
		sprintf(tmp, "\"svol2\":%d,", pDepthMarketData->AskVolume2);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["sell3"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->AskPrice3);
		sprintf(tmp, "\"sell3\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["svol3"]) {
		sprintf(tmp, "\"svol3\":%d,", pDepthMarketData->AskVolume3);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["sell4"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->AskPrice4);
		sprintf(tmp, "\"sell4\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["svol4"]) {
		sprintf(tmp, "\"svol4\":%d,", pDepthMarketData->AskVolume4);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["sell5"]) {
		CHECK_AND_ASSIGN_DOUBLE_VALUE(dst, pDepthMarketData->AskPrice5);
		sprintf(tmp, "\"sell5\":%.3lf,", dst);
		strcat(json_string, tmp);
	}

	if (map_sub_fields["svol5"]) {
		sprintf(tmp, "\"svol5\":%d,", pDepthMarketData->AskVolume5);
		strcat(json_string, tmp);
	}

	int length = strlen(json_string);
	if (length > 5) {
		json_string[length - 1] = 0x00;
	}
	strcat(json_string, "}");

	return json_string;
}

void ctp_quote::OnFrontConnected()
{
	TRACE_INFO("ctp-quote", "on front connected");

	login();
}
/* ctp-quote sdk can reconnect to ctp server automatically, so we don't need to try outside */
void ctp_quote::OnFrontDisconnected(int nReason)
{
	TRACE_SYSTEM("ctp-quote", "on front disconnected, reason:%d, ", nReason);
}

void ctp_quote::OnHeartBeatWarning(int nTimeLapse)
{
	TRACE_INFO("ctp-quote", "on heartbeat warning, time lapse:%d", nTimeLapse);
}

void ctp_quote::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	TRACE_INFO("ctp-quote", "on user login response");

	if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_RSP_USERLOGIN_ERROR,
				"OnRspUserLogin username:'%s' ErrorID:%d ErrorMsg:'%s'",
				params_.ctp_user.c_str(), pRspInfo->ErrorID, gbk_to_utf8(pRspInfo->ErrorMsg).c_str());
	}
	else {
		subscribe_quote();
	}
}

void ctp_quote::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	TRACE_INFO("ctp-quote", "on user login out response");

	if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_RSP_USERLOGOUT_ERROR,
				"OnRspUserLogout username:'%s' ErrorID:%d ErrorMsg:'%s'",
				params_.ctp_user.c_str(), pRspInfo->ErrorID, gbk_to_utf8(pRspInfo->ErrorMsg).c_str());
	}
}

void ctp_quote::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_RSP_ERROR,
			"OnRspError  ErrorID:%d ErrorMsg:'%s'",
			pRspInfo->ErrorID, gbk_to_utf8(pRspInfo->ErrorMsg).c_str());
}

void ctp_quote::OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	TRACE_INFO("ctp-quote", "on subscribe market data response");

	if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_RSP_SUBMARKETDATA_ERROR,
				"OnRspSubMarketData Instrument:'%s' ErrorID:%d ErrorMsg:'%s'",
				pSpecificInstrument != NULL ? pSpecificInstrument->InstrumentID : "",
				pRspInfo->ErrorID, gbk_to_utf8(pRspInfo->ErrorMsg).c_str());
	}
}

void ctp_quote::OnRspUnSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	TRACE_INFO("ctp-quote", "on unsubscribe market data response");

	if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_RSP_UNSUBMARKETDATA_ERROR,
				"OnRspUnSubMarketData Instrument:'%s' ErrorID:%d ErrorMsg:'%s'",
				pSpecificInstrument != NULL ? pSpecificInstrument->InstrumentID : "",
				pRspInfo->ErrorID, gbk_to_utf8(pRspInfo->ErrorMsg).c_str());
	}
}

void ctp_quote::OnRspSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	TRACE_INFO("ctp-quote", "on subscribe for quote response");

	if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_RSP_SUBFORQUOTE_ERROR,
				"OnRspSubForQuoteRsp Instrument:'%s' ErrorID:%d ErrorMsg:'%s'",
				pSpecificInstrument != NULL ? pSpecificInstrument->InstrumentID : "",
				pRspInfo->ErrorID, gbk_to_utf8(pRspInfo->ErrorMsg).c_str());
	}
}

void ctp_quote::OnRspUnSubForQuoteRsp(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	TRACE_INFO("ctp-quote", "on unsubscribe for quote response");

	if (pRspInfo != NULL && pRspInfo->ErrorID != 0) {
		TRACE_ERROR("ctp-quote", NAUT_CTPQUOTE_E_RSP_UNSUBFORQUOTE_ERROR,
				"OnRspUnSubForQuoteRsp Instrument:'%s' ErrorID:%d ErrorMsg:'%s'",
				pSpecificInstrument != NULL ? pSpecificInstrument->InstrumentID : "",
				pRspInfo->ErrorID, gbk_to_utf8(pRspInfo->ErrorMsg).c_str());
	}
}

void ctp_quote::OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData)
{
	TRACE_INFO("ctp-quote", "on receive depth market data");

	assert(pDepthMarketData != NULL);

	on_receive_quote_data(pDepthMarketData);
}

void ctp_quote::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
{
	TRACE_INFO("ctp-quote", "on receive quote data");
}

std::string ctp_quote::gbk_to_utf8(const char* text)
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
    return "";
}

char* ctp_quote::change_date_format(char* outdate, char* inputdate)
{
    if(outdate == NULL || inputdate == NULL) {
        return NULL;
    }
    for(int i = 0, j = 0; i < 11; i++) {
        if(i == 4 || i == 7) {
            outdate[i] = '-';
            continue;
        }
        outdate[i] = inputdate[j];
        j++;
    }
    return outdate;
}

}
