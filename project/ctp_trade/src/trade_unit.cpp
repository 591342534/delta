/*****************************************************************************
 Nautilus Module ctp_trade Copyright (c) 2016. All Rights Reserved.

 FileName: trade_unit.cpp
 Version: 1.0
 Date: 2016.02.01

 History:
 clomy     2016.02.01   1.0     Create
 ******************************************************************************/

#include "trade_unit.h"
#include "common.h"
#include "trade_struct.h"
#include "trade_server.h"
#include "base/trace.h"
#include "base/util.h"
#include "base/base.h"
#include "iconv/iconv.h"
#include "database/unidbpool.h"
#include "progress_recorder.h"

namespace ctp
{

trade_unit::trade_unit() :
        mdpt_(NULL), tserver_(NULL), trader_(NULL), connected_(false), logined_(false), msg_event_(
                NULL), state_event_(NULL), started_(false), m_RequestId_(0), m_FrontID_(
                0), m_SessionID_(0), m_stockhold_old_buy_(0), m_stockhold_old_sell_(0)
                ,m_force_orders_started_(false), m_last_respond(base::util::clock()), m_conn_state(cs_init),m_last_qryacc_finish(true)
{
}

trade_unit::~trade_unit()
{
    stop();
}

int trade_unit::start(const trade_unit_params& params, message_dispatcher* mdpt,
        trade_server* tserver)
{
    assert(mdpt != NULL);

    if (started_) {
        return NAUT_AT_S_OK;
    }

    params_ = params;
    mdpt_ = mdpt;
    tserver_ = tserver;

    userid_ = params_.userid;
    account_ = params_.account;
    broker_ = params_.broker;

    int ret = start_internal();
    if (BSUCCEEDED(ret)) {
        started_ = true;
    } else {
        stop();
    }
    return ret;
}

void trade_unit::stop()
{
    started_ = false;
    stop_internal();
}

int trade_unit::start_internal()
{
    int ret = NAUT_AT_S_OK;

    LABEL_SCOPE_START;

    msg_event_ = new base::event();
    state_event_ = new base::event();

    if (!params_.use_simulation_flag) {
        trade_reconnect();
    } else {
        connected_ = true;
        logined_ = true;
    }
    /* start processor thread */
    processor_base::start();

    LABEL_SCOPE_END;

    end:
    return ret;
}

int trade_unit::trade_reconnect()
{
    int ret = NAUT_AT_S_OK;

    m_last_respond = base::util::clock() + 3*1000;
    m_conn_state = cs_connecting;
    /* initialize ctp server */
    if (trader_ != NULL) {
//        base::util::sleep(10000);
        trader_->Release();
        trader_ = NULL;
    }
    connected_ = false;
    logined_ = false;
    char addr[256];
    std::string spath;
    sprintf(addr, "tcp://%s:%d", params_.trade_host.c_str(),
            params_.trade_port);

    spath = "./ctp_log/" + params_.account;
    base::file::make_dirs(spath.c_str());
    spath += "/";
    trader_ = CThostFtdcTraderApi::CreateFtdcTraderApi(spath.c_str());
    trader_->RegisterFront(addr);
    trader_->SubscribePublicTopic(THOST_TERT_RESUME);
    trader_->SubscribePrivateTopic(THOST_TERT_RESUME);
    trader_->RegisterSpi(this);
    trader_->Init();
    return ret;
}

int trade_unit::stop_internal()
{
    /* stop processor thread */
    processor_base::stop();

    /* release trade messages left in the queue */
    release_messages();

    if (msg_event_ != NULL) {
        delete msg_event_;
        msg_event_ = NULL;
    }

    if (state_event_ != NULL) {
        delete state_event_;
        state_event_ = NULL;
    }

    map_int_req_fun_.clear();

    if (trader_ != NULL) {
        trader_->Release();
        trader_ = NULL;
    }
    return NAUT_AT_S_OK;
}

void trade_unit::post(atp_message& msg)
{
    /* ignore query if current server is disconnected */
    if (connected_ || msg.type == ATP_MESSAGE_TYPE_SERVER_TRADE_REQ) {
        processor_base::post(msg);
    }

    if (started_) {
        assert(msg_event_ != NULL);
        msg_event_->set();
    }
}

void trade_unit::run()
{
    atp_message msg;
    std::string force_start_time;
//    long int_reconnect = 0;
//    long int_connect = 0;
    while (is_running_) {
        if (!params_.use_simulation_flag) {
            force_start_time = base::util::local_time_string();
            std::string trade_day = tserver_->get_comm_holiday()->get_trade_day();
            if(tserver_->get_comm_holiday()->isholiday(base::util::string_to_datestamp((char*)trade_day.c_str())) == true){
                base::util::sleep(10000);
                continue;
            }

            if(((force_start_time >= "08:30:00" && force_start_time <="16:00:00")
                    || (force_start_time >= "20:30:00" || force_start_time <="03:00:00") ) )
            {
                //没有位处理的单子时，不发送请求，含：所有单子都已经回报
                if (MakeActive() == false){
//                    TRACE_SYSTEM(AT_TRACE_TAG, "begin reconnect now~~~");
                    if (base::util::clock() - m_last_respond > 10*1000) {   //20秒
                        // for test
//                        TRACE_SYSTEM(AT_TRACE_TAG,"int_reconnect:%ld", int_reconnect++);
                        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED, "error 实盘线程 交易太久(%lld秒)没有通讯了，删除重建 \n", (base::util::clock() - m_last_respond)/1000);
                        trade_reconnect();
                        base::util::sleep(1000);
                    }

                    if(!connected_ || !logined_){
                        TRACE_WARNING(AT_TRACE_TAG, "reconnect failed will retray now~~~");
                        base::util::sleep(1000);
                        continue;
                    }
//                    TRACE_SYSTEM(AT_TRACE_TAG, "end reconnect now~~~");
                } else {
                    // for test
//                    TRACE_SYSTEM(AT_TRACE_TAG,"int_connect:%ld", int_connect++);
//                    connected_ = false;
//                    base::util::sleep(5000);
                }

            }
        }

        if (!connected_ || !logined_ || !started_ || !tserver_->get_server_start_flag()) {
            state_event_->wait(50);
            continue;
        }

        int ret = get(msg);
        if (ret != 0) {
            msg_event_->reset();
            msg_event_->wait(20);
            continue;
        }

        ref_dictionary* rd = (ref_dictionary*) msg.param1;
        assert(rd != NULL);

        base::dictionary* dict = rd->get();
        std::string cmd = (*dict)["cmd"].string_value();

        TRACE_SYSTEM(AT_TRACE_TAG, "trade unit prepare to send order, %s",
                dict->to_string().c_str());

        ret = NAUT_AT_S_OK;

        if (cmd == ATPM_CMD_ENTRUST) {
            ret = prepare_entrust(*dict);
        } else if (cmd == ATPM_CMD_WITHDRAW) {
            ret = prepare_withdraw(*dict);
        } else if (cmd == ATPM_CMD_QRY_ACCOUNT) {
            ret = prepare_qryaccount(*dict);
        } else if (cmd == ATPM_CMD_QRY_DEAL
                || cmd == ATPM_CMD_QRY_CHECKER_DEAL) {
            ret = prepare_qrydeal(*dict);
        } else if (cmd == ATPM_CMD_QRY_OPENINTER) {
            ret = prepare_qryopeninter(*dict);
        } else if (cmd == ATPM_CMD_QRY_ENTRUST
                || cmd == ATPM_CMD_QRY_CHECKER_ENTRUST) {
            ret = prepare_qryentrust(*dict);
        } else {
            ret = NAUT_AT_E_UNKNOWN_TRADE_CMD;
        }

        if (BSUCCEEDED(ret)) {
            if (cmd == ATPM_CMD_ENTRUST || cmd == ATPM_CMD_WITHDRAW) {
                progress_recorder::shared_instance().record_progress(
                        (*dict)["broker"].string_value().c_str(),
                        (*dict)["account"].string_value().c_str(),
                        (*dict)["orderid"].string_value().c_str(),
                        (*dict)["msg_index"].integer_value(),
                        ORDER_SENDDING_TO_SERVER);
            }

//			assert(trader_ != NULL);
//			int result = trader_->ShZdSendInfoToTrade(&zmsg);
//			if (result != 0) {
//				zmsg.SetTag(ZD_TRADE_PWD, "");
//				TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED,
//						"tu(%s): send trade info to server failed '%d', will retry later, '%s'",
//						account_.c_str(), result, zmsg._GetAllString().c_str());
//				ret = NAUT_AT_E_SEND_TRADEINFO_FAILED;
//			}
//			else {
//				if (cmd == ATPM_CMD_ENTRUST || cmd == ATPM_CMD_WITHDRAW) {
//					progress_recorder::shared_instance().record_progress(
//							(*dict)["broker"].string_value().c_str(),
//							(*dict)["account"].string_value().c_str(),
//							(*dict)["orderid"].string_value().c_str(),
//							(*dict)["msg_index"].integer_value(), ORDER_FINISH);
//				}
//			}
        } else {
            TRACE_SYSTEM(AT_TRACE_TAG,
                    "trade unit prepare order failed, ret: %d, dict: %s", ret,
                    dict->to_string().c_str());
        }

        /* notify if the order is failed */
        if (BFAILED(ret)) {
            base::dictionary* pdict = new base::dictionary(*dict);
            ref_dictionary* rd1 = new ref_dictionary(pdict);

            (*pdict)["real_cmd"] = (*pdict)["cmd"].string_value();
            (*pdict)["cmd"] = ATPM_CMD_ERROR;
            (*pdict)["error_code"] = get_response_error_code(ret);
            (*pdict)["error_msg"] = get_response_error_msg((*pdict)["error_code"].integer_value());

            atp_message msg1;
            if (msg.type == ATP_MESSAGE_TYPE_SERVER_TRADE_REQ) {
                msg1.type = ATP_MESSAGE_TYPE_SERVER_TRADE_RSP;
            } else {
                msg1.type = ATP_MESSAGE_TYPE_SERVER_QUERY_RSP;
            }
            msg1.param1 = (void*) rd1;
            mdpt_->dispatch_message(msg1);
        }

        rd->release();
    }
}

bool trade_unit::MakeActive() {
    long cur_millsec = base::util::clock();

    if (m_conn_state == cs_connecting && cur_millsec - m_last_respond < 5*1000)
    {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED, "连接检查(%s) >> 正在链接中..... \n", account_.c_str());
        return false;
    }
    if (connected_ == false){
        m_last_respond = 0;
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED, "连接检查(%s) >> 没有连接 \n", account_.c_str());
        return false;
    }
    if (cur_millsec - m_last_respond > 10*1000){
        connected_ = false;
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED, "连接检查(%s) >> 超时(%lld),重新连接 \n", account_.c_str(), cur_millsec - m_last_respond);
        m_last_respond = 0;
        return false;
    } else if (cur_millsec - m_last_respond > 6*1000){
//        TRACE_WARNING(AT_TRACE_TAG, "MakeActive 超时，确认是否连接 %lld(ms)\n", cur_millsec - m_last_respond);
        base::dictionary dict;
        prepare_qryaccount(dict);
        m_last_respond += 1000;   //不可以去掉，免得立即再次进来
        return false;
    }

    return true;
}

void trade_unit::release_messages()
{
    atp_message msg;
    while (get(msg) == 0) {
        ref_dictionary* rd = (ref_dictionary*) msg.param1;
        if (rd != NULL) {
            rd->release();
        }
    }
}

int trade_unit::login()
{
    if (!connected_) {
        return NAUT_AT_E_CTPTRADE_NOT_CONNECTED;
    }

    int ret = NAUT_AT_S_OK;

    CThostFtdcReqUserLoginField loginReq;
    memset(&loginReq, 0, sizeof(loginReq));

    strcpy(loginReq.BrokerID, params_.broker.c_str());

    strcpy(loginReq.UserID, params_.userid.c_str());
    strcpy(loginReq.Password, params_.password.c_str());

    int result = trader_->ReqUserLogin(&loginReq, GetRequestId());

    if (result != 0) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED,
                "tu(%s): send login data to trade server failed, ret: %d, errormsg: %s",
                account_.c_str(), result, GetRetErrorMsg(result).c_str());
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED,
                end);
    }

    end: return ret;
}

int trade_unit::logout()
{
    if (!connected_) {
        return NAUT_AT_E_CTPTRADE_NOT_CONNECTED;
    }

    int ret = NAUT_AT_S_OK;

    CThostFtdcUserLogoutField logoutReq;
    memset(&logoutReq, 0, sizeof(logoutReq));

    strcpy(logoutReq.BrokerID, params_.broker.c_str());
    strcpy(logoutReq.UserID, params_.userid.c_str());

    int result = trader_->ReqUserLogout(&logoutReq, GetRequestId());

    if (result != 0) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED,
                "tu(%s): send login data to trade server failed, ret: %d, errormsg: %s",
                account_.c_str(), result, GetRetErrorMsg(result).c_str());
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED,
                end);
    }

    end: return ret;
}

int trade_unit::prepare_entrust(const base::dictionary& dict)
{
    int ret = NAUT_AT_S_OK;
    naut::DBCONNECT* db_conn = NULL;
    LABEL_SCOPE_START;
    long order_time = dict["order_time"].integer_value();
    long order_valid_time = dict["order_valid_time"].integer_value();
    long ltime = base::util::local_timestamp();
    if (order_valid_time < ltime && order_time != order_valid_time) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ORDER_EXPIRED,
                "tu(%s): entrust order is expired, '%s'", account_.c_str(),
                dict.to_string().c_str());
        ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ORDER_EXPIRED, end);
    }

    if (!params_.use_simulation_flag) {
        CThostFtdcInputOrderField orderinsert;
        memset(&orderinsert, 0, sizeof(orderinsert));
        strcpy(orderinsert.BrokerID, broker_.c_str());
        strcpy(orderinsert.UserID, userid_.c_str());
        strcpy(orderinsert.InvestorID, userid_.c_str());
        strcpy(orderinsert.InstrumentID, dict["code"].string_value().c_str());
        strcpy(orderinsert.OrderRef, dict["localno"].string_value().c_str());

        int direction = dict["direction"].int_value();
        int entrust_type = dict["entrust_type"].int_value();
        if (direction == entrust_type) {
            orderinsert.Direction = THOST_FTDC_D_Buy;
        } else {
            orderinsert.Direction = THOST_FTDC_D_Sell;
        }

        switch (entrust_type) {
            case 1:
                orderinsert.CombOffsetFlag[0] = THOST_FTDC_OF_Open;
                break;
            case 2:
                if (orderinsert.Direction == THOST_FTDC_D_Buy){
                    if(m_stockhold_old_sell_ > 0) {
                        orderinsert.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
                        m_stockhold_old_sell_--;
                    } else {
                        orderinsert.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
                    }
                }else if (orderinsert.Direction == THOST_FTDC_D_Sell){
                    if(m_stockhold_old_buy_ > 0) {
                        orderinsert.CombOffsetFlag[0] = THOST_FTDC_OF_Close;
                        m_stockhold_old_buy_--;
                    } else {
                        orderinsert.CombOffsetFlag[0] = THOST_FTDC_OF_CloseToday;
                    }
                }
                break;
            default:
                TRACE_WARNING(AT_TRACE_TAG, "entrust_type: %ld is not right", entrust_type);
                break;
        }

        orderinsert.CombHedgeFlag[0] = THOST_FTDC_HF_Speculation;
        orderinsert.VolumeTotalOriginal = dict["entrust_amount"].int_value();
        orderinsert.VolumeCondition = THOST_FTDC_VC_AV;
        orderinsert.MinVolume = 1;
        orderinsert.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
        orderinsert.IsAutoSuspend = 0;
        orderinsert.UserForceClose = 0;
        orderinsert.StopPrice = 0;
        orderinsert.ContingentCondition = THOST_FTDC_CC_Immediately;
        orderinsert.ForceCloseReason = THOST_FTDC_FCC_NotForceClose;
        // price_type: 价格类别（1 limit order 2 market order）
        if (dict["price_type"].int_value() == 1) {
            orderinsert.OrderPriceType = THOST_FTDC_OPT_AnyPrice;
            orderinsert.TimeCondition = THOST_FTDC_TC_IOC;
        } else {
            orderinsert.OrderPriceType = THOST_FTDC_OPT_LimitPrice;
            orderinsert.TimeCondition = THOST_FTDC_TC_GFD;
        }
        orderinsert.LimitPrice = dict["entrust_price"].double_value();
        orderinsert.RequestID = GetRequestId();
        int ret = trader_->ReqOrderInsert(&orderinsert, orderinsert.RequestID);
        if (ret != 0) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED,
                    "tu(%s): send orderinsert to trade server failed, ret: %d, errormsg: %s",
                    account_.c_str(), ret, GetRetErrorMsg(ret).c_str());
            ASSIGN_AND_CHECK_LABEL(ret,
                    NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED, end);
        }
        map_int_req_fun_[orderinsert.RequestID] =
                PROCESS_ENTRUST_RESULT_REQUESTID;

    } else {
        base::dictionary indict = dict;
        map_account_channel& map_channel =
                tserver_->get_map_account_channel_bs();
        string channel_key = tserver_->get_account_broker_bs_key(
                dict["broker"].string_value(), dict["account"].string_value(),
                (long) dict["direction"].integer_value());

        map_account_channel::iterator channel_iter = map_channel.find(
                channel_key);
        string tdhstr = "0";
        if (channel_iter != map_channel.end()) {
            ++(channel_iter->second.totalnum);
            if (channel_iter->second.chanelvec.size()) {
                int localpos = channel_iter->second.totalnum
                        % channel_iter->second.chanelvec.size();
                tdhstr = channel_iter->second.chanelvec[localpos];
            }
        }

        std::string entrustbs_char = "0";
        // entrust_type   direction entrustbs
        // 1    1    1
        // 1    2    2
        // 2    1    3
        // 2    2    4
        if (dict["entrust_type"].integer_value() == 1 && dict["direction"].integer_value() == 1) {
            entrustbs_char = "1";
        } else if(dict["entrust_type"].integer_value() == 1 && dict["direction"].integer_value() == 2) {
            entrustbs_char = "2";
        } else if(dict["entrust_type"].integer_value() == 2 && dict["direction"].integer_value() == 1) {
            entrustbs_char = "3";
        } else if(dict["entrust_type"].integer_value() == 2 && dict["direction"].integer_value() == 2) {
            entrustbs_char = "4";
        }

            char sql[2048];
        memset(sql, 0, 2048);
        sprintf(sql,
                "insert into %s(account,entrustbs,exchangetype,stockcode,amount,entrustprice,entrustdate,entrusttime,broker,tdh,orderlevel,checkflag,zd_username) "
                        " values('%s','%s','%s','%s',%.0f,%f,'%s','%s','%s','%s',%ld, %ld, '%s')",
                params_.autotrade_entrust_tbl_name.c_str(),
                dict["account"].string_value().c_str(),
                entrustbs_char.c_str(),
                dict["excode"].string_value().c_str(),
                dict["code"].string_value().c_str(),
                dict["entrust_amount"].double_value(),
                dict["entrust_price"].double_value(),
                base::util::date_string(order_time).c_str(),
                base::util::time_string(order_time).c_str(),
                dict["broker"].string_value().c_str(), tdhstr.c_str(),
                dict["orderlevel"].integer_value(),
                dict["is_retry"].integer_value(),
                dict["orderid"].string_value().c_str());

        db_conn = tserver_->get_risk_conn_pool()->getconn();

        if (db_conn != NULL && db_conn->_conn->execute(sql)) {
            memset(sql, 0, 2048);
            sprintf(sql, "select last_insert_id() as last_id;");
            if (!db_conn->_conn->query(sql)) {
                TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_QUERY_SERVER_CONFIG_FAILED,
                        "query last_insert_id() failed, db error: (%d:%s)",
                        db_conn->_conn->get_errno(),
                        db_conn->_conn->get_error().c_str());
                ASSIGN_AND_CHECK_LABEL(ret,
                        NAUT_AT_E_QUERY_SERVER_CONFIG_FAILED, end);
            }

            if (db_conn->_conn->get_count() == 0
                    || !db_conn->_conn->fetch_row()) {
                TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_SERVER_CONFIG_NOT_EXIST,
                        " last_insert_id() is not exist");
                ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_SERVER_CONFIG_NOT_EXIST,
                        end);
            }

            indict["systemno"] = (long) db_conn->_conn->get_long("last_id");
            indict["at_error"] = (long) AT_ERROR_NONE;
        } else {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
                    "insert order failed, sql: '%s'", sql);
            indict["at_error"] = AT_ERROR_DATABASE_FAILED;
        }
        indict["order_status"] = (long) 0;
        process_entrust_result(indict);
    }
    LABEL_SCOPE_END;

    end: if (db_conn != NULL) {
        tserver_->get_risk_conn_pool()->retconn(db_conn);
    }

    return ret;
}

int trade_unit::prepare_withdraw(const base::dictionary& dict)
{
    int ret = NAUT_AT_S_OK;
    naut::DBCONNECT* db_conn = NULL;
    LABEL_SCOPE_START;

    if (!params_.use_simulation_flag) {
        long order_time = dict["order_time"].integer_value();
        long order_valid_time = dict["order_valid_time"].integer_value();
        long ltime = base::util::local_timestamp();
        if (order_valid_time < ltime && order_time != order_valid_time) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ORDER_EXPIRED,
                    "tu(%s): entrust order is expired, '%s'", account_.c_str(),
                    dict.to_string().c_str());
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_ORDER_EXPIRED, end);
        }

        CThostFtdcInputOrderActionField InputOrderAction;
        memset(&InputOrderAction, 0, sizeof(InputOrderAction));

        strcpy(InputOrderAction.BrokerID, broker_.c_str());
        strcpy(InputOrderAction.InvestorID, userid_.c_str());
        InputOrderAction.FrontID = m_FrontID_;
        InputOrderAction.SessionID = m_SessionID_;
        strcpy(InputOrderAction.InstrumentID,
                dict["code"].string_value().c_str());
        InputOrderAction.ActionFlag = THOST_FTDC_AF_Delete;
        strcpy(InputOrderAction.OrderSysID,
                dict["entrustno"].string_value().c_str());
        strcpy(InputOrderAction.OrderRef,
                dict["localno"].string_value().c_str());
        InputOrderAction.RequestID = GetRequestId();

        int ret = trader_->ReqOrderAction(&InputOrderAction,
                InputOrderAction.RequestID);
        m_last_respond = base::util::clock();

        if (ret != 0) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED,
                    "tu(%s): withdraw to trade server failed, ret: %d, errormsg: %s",
                    account_.c_str(), ret, GetRetErrorMsg(ret).c_str());
            ASSIGN_AND_CHECK_LABEL(ret,
                    NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED, end);
        }
        map_int_req_fun_[InputOrderAction.RequestID] =
                PROCESS_WITHDRAW_RESULT_REQUESTID;
    } else {
        base::dictionary indict = dict;
        char sql[2048];
        memset(sql, 0, 2048);
        sprintf(sql,
                "insert into %s (account,broker, entrustno,tradedate,inserttime) "
                        " values('%s','%s', %ld, '%s',NOW())",
                params_.autotrade_withdraw_tbl_name.c_str(),
                dict["account"].string_value().c_str(),
                dict["broker"].string_value().c_str(),
                dict["entrustno"].integer_value(),
                dict["tradedate"].string_value().c_str());

        db_conn = tserver_->get_risk_conn_pool()->getconn();
        if (db_conn != NULL && db_conn->_conn->execute(sql)) {
            indict["at_error"] = (long) AT_ERROR_NONE;
        } else {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_EXCUTE_DB_FAILED,
                    "insert order failed, sql: '%s'", sql);
            indict["at_error"] = AT_ERROR_DATABASE_FAILED;
        }
        process_withdraw_result(indict);
    }
    LABEL_SCOPE_END;

    end: if (db_conn != NULL) {
        tserver_->get_risk_conn_pool()->retconn(db_conn);
    }
    return ret;

}

int trade_unit::prepare_qryaccount(const base::dictionary& dict)
{
    int ret = NAUT_AT_S_OK;
    LABEL_SCOPE_START;

    if (!params_.use_simulation_flag) {
    	if((m_last_qryacc_finish == false) && (base::util::local_timestamp() - m_last_qryacc < 30)){
    		TRACE_SYSTEM(AT_TRACE_TAG,"tu(%s): last query at %s has not finished !please send you order some time late",
    		                    account_.c_str(), base::util::time_string(m_last_qryacc).c_str());
    		return ret;
    	}
        CThostFtdcQryTradingAccountField trdfield;
        memset(&trdfield, 0, sizeof(CThostFtdcQryTradingAccountField));
        strcpy(trdfield.InvestorID, userid_.c_str());
        strcpy(trdfield.BrokerID, broker_.c_str());
        int requestid = GetRequestId();
        int ret = trader_->ReqQryTradingAccount(&trdfield, requestid);
        if (ret != AT_ERROR_NONE) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED,
                    "tu(%s): qryaccount to trade server failed, ret: %d, errormsg: %s",
                    account_.c_str(), ret, GetRetErrorMsg(ret).c_str());
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED, end);
        }
        map_int_req_fun_[requestid] = PROCESS_QRYACCOUNT_RESULT_REQUESTID;
        m_last_qryacc = base::util::local_timestamp();
        m_last_qryacc_finish = false;
    } else {
        base::dictionary rdict(dict);

        rdict["userid"] = userid_;
        rdict["account"] = account_;
        rdict["canuse_fund"] = (long)10000000000;
        rdict["frozen_fund"] = (long)0;
        rdict["left_fund"] = (long)10000000000;
        rdict["fee"] = (long)0;
        rdict["deposit"] = (long)0;
        rdict["equity"] = (long)0;
        rdict["at_error"] = (long) AT_ERROR_CTP_NONE;
        process_qryaccount_result(rdict);
    }
    LABEL_SCOPE_END;

end:
    return ret;
}

int trade_unit::prepare_qrydeal(const base::dictionary& dict)
{
    int ret = NAUT_AT_S_OK;
    naut::DBCONNECT* db_conn = NULL;
    LABEL_SCOPE_START;
    if (!params_.use_simulation_flag) {
        CThostFtdcQryTradeField QryTrade;
        memset(&QryTrade, 0, sizeof(QryTrade));
        strcpy(QryTrade.InvestorID, userid_.c_str());
        strcpy(QryTrade.BrokerID, broker_.c_str());
//        strcpy(QryTrade.ExchangeID, dict["excode"].string_value().c_str());
//        if (!dict["dealno"].string_value().empty()) {
//            strcpy(QryTrade.TradeID, dict["dealno"].string_value().c_str());
//        }
        int requestid = GetRequestId();
        ret = trader_->ReqQryTrade(&QryTrade, requestid);
        m_last_respond = base::util::clock();
        if (ret != AT_ERROR_NONE) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED,
                    "tu(%s): qrydeal to trade server failed, ret: %d, errormsg: %s, dict:[%s]",
                    account_.c_str(), ret, GetRetErrorMsg(ret).c_str(), dict.to_string().c_str());
            ASSIGN_AND_CHECK_LABEL(ret,
                    NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED, end);
        }
        map_int_req_fun_[requestid] = PROCESS_QRYDEAL_RESULT_REQUESTID;
    } else {
        base::dictionary indict = dict;
        /* query deal */
        char sql[512];
        sprintf(sql,
                "select * from %s where broker = '%s' and account = '%s' and entrustid = %ld and entrustbs = %ld",
                tserver_->get_autotrade_deal_tbl_name().c_str(),
                dict["broker"].string_value().c_str(),
                dict["account"].string_value().c_str(),
                dict["systemno"].integer_value(),
                dict["entrust_type"].integer_value());
        db_conn = tserver_->get_risk_conn_pool()->getconn();
        if (db_conn == NULL || !db_conn->_conn->query(sql)) {
            TRACE_ERROR(AT_TRACE_TAG, AT_ERROR_CONNECT_SERVER_FAILED,
                    "query server config failed, db error: (%d:%s)",
                    db_conn->_conn->get_errno(),
                    db_conn->_conn->get_error().c_str());
            ASSIGN_AND_CHECK_LABEL(ret, AT_ERROR_CONNECT_SERVER_FAILED, end);
        }

        if (db_conn->_conn->get_count() == 0 || !db_conn->_conn->fetch_row()) {
            TRACE_WARNING(AT_TRACE_TAG,
                    "this deal doesn't have done now, please true later!  dict: [%s] , sql:[ %s ]",
                    dict.to_string().c_str(), sql);
            goto end;
        }

        indict["broker"] = db_conn->_conn->get_string("broker");
        indict["userid"] = db_conn->_conn->get_string("account");
        indict["account"] = db_conn->_conn->get_string("account");
        indict["entrustno"] = (long) db_conn->_conn->get_long("entrustno");
        indict["dealno"] = db_conn->_conn->get_string("dealno");
        indict["systemno"] = db_conn->_conn->get_string("entrustid");
        indict["code"] = db_conn->_conn->get_string("stockcode");
        indict["code_name"] = db_conn->_conn->get_string("stockname");
        indict["direction"] = (long) db_conn->_conn->get_long("entrustbs");
        indict["entrust_type"] = (long) db_conn->_conn->get_long("entrustbs");
        indict["deal_amount"] = (long) db_conn->_conn->get_long("dealamount");
        indict["deal_price"] = (double) db_conn->_conn->get_double("dealprice");
        indict["deal_date"] = base::util::local_date_string();
        indict["deal_time"] = db_conn->_conn->get_string("dealtime");
        indict["deal_fund"] = db_conn->_conn->get_string("dealfund");
        string deal_reasultstr = db_conn->_conn->get_string("dealresult");
        if (deal_reasultstr == "1") {
            indict["at_error"] = AT_ERROR_NONE;
        } else {
            indict["at_error"] =
                    AT_ERROR_ORDER_HAS_BEEN_WITHDRAWED_OR_IS_FAILED;
        }
        process_qrydeal_result(indict);
    }
    LABEL_SCOPE_END;
    end: if (db_conn != NULL) {
        tserver_->get_risk_conn_pool()->retconn(db_conn);
    }

    return ret;
}

int trade_unit::prepare_qryopeninter(const base::dictionary& dict)
{
    int ret = NAUT_AT_S_OK;
    LABEL_SCOPE_START;

    if (!params_.use_simulation_flag) {
        CThostFtdcQryInvestorPositionField QryInvestorPosition;
        memset(&QryInvestorPosition, 0, sizeof(QryInvestorPosition));
        strcpy(QryInvestorPosition.InvestorID, userid_.c_str());
        strcpy(QryInvestorPosition.BrokerID, broker_.c_str());
        if (!dict["code"].is_null()) {
            strcpy(QryInvestorPosition.InstrumentID,
                    dict["code"].string_value().c_str());
        }
        int requestid = GetRequestId();
        int ret = trader_->ReqQryInvestorPosition(&QryInvestorPosition, requestid);

        if (ret != AT_ERROR_NONE) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED,
                    "tu(%s): qryaccount to trade server failed, ret: %d, errormsg: %s",
                    account_.c_str(), ret, GetRetErrorMsg(ret).c_str());
            ASSIGN_AND_CHECK_LABEL(ret, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED, end);
        }
        m_stockhold_old_buy_ = 0;
        m_stockhold_old_sell_ = 0;
        map_int_req_fun_[requestid] = PROCESS_QRYOPENINER_RESULT_REQUESTID;
    } else {
        TRACE_SYSTEM(AT_TRACE_TAG, "receive aryopeninter dict:[ %s ]", dict.to_string().c_str());
    }
    LABEL_SCOPE_END;

end:
    return ret;
}

int trade_unit::prepare_qryentrust(const base::dictionary& dict)
{
    int ret = NAUT_AT_S_OK;
    naut::DBCONNECT* db_conn = NULL;
    LABEL_SCOPE_START;

    if (!params_.use_simulation_flag) {
        CThostFtdcQryOrderField QryOrder;
        memset(&QryOrder, 0, sizeof(QryOrder));
        strcpy(QryOrder.BrokerID, broker_.c_str());
        strcpy(QryOrder.InvestorID, userid_.c_str());
//        strcpy(QryOrder.InstrumentID, dict["code"].string_value().c_str());
//        strcpy(QryOrder.ExchangeID, dict["excode"].string_value().c_str());
//        strcpy(QryOrder.OrderSysID, dict["localno"].string_value().c_str());
        //strcpy(QryOrder.InsertTimeStart, "09:00:00");
        //strcpy(QryOrder.InsertTimeEnd, "16:00:00");

        int requestid = GetRequestId();
        int ret = trader_->ReqQryOrder(&QryOrder, requestid);
        m_last_respond = base::util::clock();
        if (ret != AT_ERROR_NONE) {
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED,
                    "tu(%s): qryentrust to trade server failed, ret: %d, errormsg: %s",
                    account_.c_str(), ret, GetRetErrorMsg(ret).c_str());
            ASSIGN_AND_CHECK_LABEL(ret,
                    NAUT_AT_E_CTPTRADE_SEND_TRADE_INFO_FAILED, end);
        }
        map_int_req_fun_[requestid] = PROCESS_QRYOPENINER_RESULT_REQUESTID;
    } else {
        base::dictionary indict = dict;
        /* query entrust */
        char sql[512];
        sprintf(sql, "select * from %s where zd_username = '%s' ",
                params_.autotrade_entrust_tbl_name.c_str(),
                dict["orderid"].string_value().c_str());
        db_conn = tserver_->get_risk_conn_pool()->getconn();
        if (db_conn == NULL || !db_conn->_conn->query(sql)) {
            TRACE_ERROR(AT_TRACE_TAG, AT_ERROR_CONNECT_SERVER_FAILED,
                    "query server config failed, db error: (%d:%s), sql:[%s]",
                    db_conn->_conn->get_errno(),
                    db_conn->_conn->get_error().c_str(), sql);
            indict["at_error"] = AT_ERROR_CONNECT_SERVER_FAILED;
            ASSIGN_AND_CHECK_LABEL(ret, AT_ERROR_CONNECT_SERVER_FAILED, end);
        }

        if (db_conn->_conn->get_count() == 0 || !db_conn->_conn->fetch_row()) {
            TRACE_ERROR(AT_TRACE_TAG, AT_ERROR_DEAL_NOT_EXIST,
                    "query usrname [%s] do not exist!!!",
                    dict.to_string().c_str());
            indict["at_error"] = AT_ERROR_ORDERID_NOT_EXIST;
            ASSIGN_AND_CHECK_LABEL(ret, AT_ERROR_ORDERID_NOT_EXIST, end);
        }
        indict["systemno"] = (long) db_conn->_conn->get_long("entrustid");
        indict["order_status"] = (long) db_conn->_conn->get_long("status") + 1;
        indict["broker"] = db_conn->_conn->get_string("broker");
        indict["account"] = db_conn->_conn->get_string("account");
        indict["userid"] = db_conn->_conn->get_string("userid");
        indict["direction"] = db_conn->_conn->get_string("entrustbs");
        indict["entrust_type"] = db_conn->_conn->get_string("entrust_type");
        indict["excode"] = db_conn->_conn->get_string("exchangetype");
        indict["code"] = db_conn->_conn->get_string("stockcode");
        indict["entrust_amount"] = (long) db_conn->_conn->get_long("amount");
        indict["entrust_price"] = (double) db_conn->_conn->get_double(
                "entrustprice");

        string datetime = db_conn->_conn->get_string("entrustdate");
        datetime += " ";
        datetime += db_conn->_conn->get_string("entrusttime");
        indict["order_time"] = base::util::string_to_timestamp(
                datetime.c_str());
        indict["entrustno"] = (long) db_conn->_conn->get_long("entrustno");
        indict["orderlevel"] = (long) db_conn->_conn->get_long("orderlevel");
        indict["is_retry"] = (long) db_conn->_conn->get_long("checkflag");
        indict["orderid"] = db_conn->_conn->get_string("zd_username");
        indict["at_error"] = AT_ERROR_NONE;
        process_qryentrust_result(indict);
    }

    LABEL_SCOPE_END;
    end: if (db_conn != NULL) {
        tserver_->get_risk_conn_pool()->retconn(db_conn);
    }

    return ret;
}

// entrustno <-> OrderSysID
// systemno  <-> RequestID
// localno   <-> OrderRef

int trade_unit::process_entrust_result(CThostFtdcOrderField *pOrder,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    base::dictionary* pdict = new base::dictionary();
    base::dictionary& dict = *pdict;

    dict["cmd"] = ATPM_CMD_ENTRUST_RESPONSE;
    dict["userid"] = userid_;
    dict["account"] = account_;
    dict["broker"] = broker_;
    dict["systemno"] = pOrder->RequestID;
    dict["entrustno"] = pOrder->OrderSysID;
    dict["localno"] = pOrder->OrderRef;
    dict["code"] = pOrder->InstrumentID;
    if (bIsLast) {
        dict["result_complete"] = "1";
    } else {
        dict["result_complete"] = "0";
    }

    if (IsErrorRspInfo(pRspInfo)) {
        dict["at_error"] = (long) pRspInfo->ErrorID;
    } else {
        switch (pOrder->CombOffsetFlag[0]) {
             case '0':
                 dict["entrust_type"] = (long) 1;
                 break;
             case '1':
             case '2':
             case '3':
             case '4':
             case '5':
             case '6':
                 dict["entrust_type"] = (long) 2;
                 break;
             default:
                 TRACE_WARNING(AT_TRACE_TAG, "entrust_type: %c is not right", pOrder->CombOffsetFlag[0]);
                 break;
         }

         if (pOrder->Direction == THOST_FTDC_D_Buy) {
             dict["direction"] = (long) 1;
         } else if(pOrder->Direction == THOST_FTDC_D_Sell) {
             dict["direction"] = (long) 2;
         }

        dict["entrust_amount"] = pOrder->VolumeTotalOriginal;
        dict["entrust_price"] = pOrder->LimitPrice;
        char datebuf[11];
        memset(datebuf, 0, sizeof(datebuf));
        ChangeDateFormat(datebuf, pOrder->InsertDate);
        dict["entrust_date"] = datebuf;
        dict["entrust_time"] = pOrder->InsertTime;

        if(pOrder->OrderStatus == THOST_FTDC_OST_Canceled) {
            dict["at_error"] = (long) AT_ERROR_WITHDRAW_FROM_EXCODE;
        } else {
            dict["at_error"] = (long) AT_ERROR_NONE;
        }
    }

    ref_dictionary* rd = new ref_dictionary(pdict);
    atp_message amsg;
    amsg.type = ATP_MESSAGE_TYPE_SERVER_TRADE_RSP;
    amsg.param1 = (void*) rd;
    mdpt_->dispatch_message(amsg);

    return NAUT_AT_S_OK;
}

//int trade_unit::process_entrust_capital_result(CShZdMessage& msg)
//{
//	if (pRspInfo->ErrorID == AT_ERROR_NONE) {
//		return NAUT_AT_S_OK;
//	}
//
//	base::dictionary* pdict = new base::dictionary();
//	base::dictionary& dict = *pdict;
//
//	dict["cmd"] = ATPM_CMD_ENTRUST_CAPITAL_RESPONSE;
//
//	ref_dictionary* rd = new ref_dictionary(pdict);
//
//	atp_message amsg;
//	amsg.type = ATP_MESSAGE_TYPE_SERVER_TRADE_RSP;
//	amsg.param1 = (void*)rd;
//	mdpt_->dispatch_message(amsg);
//
//	return NAUT_AT_S_OK;
//}

// entrustno <-> OrderSysID
// dealno    <-> TradeID
// localno   <-> OrderRef

int trade_unit::process_entrust_deal_result(CThostFtdcTradeField *pTrade)
{
    base::dictionary* pdict = new base::dictionary();
    base::dictionary& dict = *pdict;

    dict["cmd"] = ATPM_CMD_DEAL;
    dict["userid"] = userid_;
    dict["account"] = account_;
    dict["broker"] = pTrade->BrokerID;
    dict["dealno"] = pTrade->TradeID;
    dict["localno"] = pTrade->OrderRef;
    dict["entrustno"] = pTrade->OrderSysID;
    dict["excode"] = pTrade->ExchangeID;
    dict["code"] = pTrade->InstrumentID;

    switch (pTrade->OffsetFlag) {
        case '0':
            dict["entrust_type"] = (long) 1;
            break;
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
            dict["entrust_type"] = (long) 2;
            break;
        default:
            TRACE_WARNING(AT_TRACE_TAG, "entrust_type: %c is not right", pTrade->OffsetFlag);
            break;
    }

    if (pTrade->Direction == THOST_FTDC_D_Buy) {
        dict["direction"] = (long) 1;
    } else if(pTrade->Direction == THOST_FTDC_D_Sell) {
        dict["direction"] = (long) 2;
    }

    dict["entrust_price"] = pTrade->PriceSource;
    dict["deal_amount"] = pTrade->Volume;
    dict["deal_price"] = pTrade->Price;

    char datebuf[11];
    memset(datebuf, 0, sizeof(datebuf));
    ChangeDateFormat(datebuf, pTrade->TradeDate);
    dict["deal_date"] = datebuf;
    dict["deal_time"] = pTrade->TradeTime;
//	dict["deal_fee"] = msg.GetString(ZD_FEE);
    dict["at_error"] = (long) AT_ERROR_NONE;

//	dict["change_date"] = pRspInfo->TradingDay;
    dict["deal_type"] = pTrade->TradeType;

    ref_dictionary* rd = new ref_dictionary(pdict);

    atp_message amsg;
    amsg.type = ATP_MESSAGE_TYPE_SERVER_TRADE_RSP;
    amsg.param1 = (void*) rd;
    mdpt_->dispatch_message(amsg);

    return NAUT_AT_S_OK;
}

//int trade_unit::process_order_status_result(CShZdMessage& msg)
//{
//	if (strcmp(msg.GetString(ZD_INFO_OVER), "1") == 0) {
//		return NAUT_AT_S_OK;
//	}
//
//	base::dictionary* pdict = new base::dictionary();
//	base::dictionary& dict = *pdict;
//
//	dict["cmd"] = ATPM_CMD_ORDER_STATUS_RESPONSE;
//	dict["userid"] = userid_;
//	dict["account"] = account_;
//	dict["systemno"] = msg.GetString(ZD_SYSTEM_NO);
//	dict["localno"] = msg.GetString(ZD_LOCAL_NO);
//	dict["entrustno"] = msg.GetString(ZD_ORDER_NO);
//	dict["code"] = msg.GetString(ZD_CODE);
//	dict["excode"] = msg.GetString(ZD_EXCHANGE_CODE);
//	dict["entrust_amount"] = msg.GetString(ZD_ORDER_NUMBER);
//	dict["entrust_price"] = msg.GetString(ZD_ORDER_NUMBER);
//	dict["deal_amount"] = msg.GetString(ZD_FILLED_NUMBER);
//	dict["canceled"] = msg.GetString(ZD_IS_CANCELED);
//
//	ref_dictionary* rd = new ref_dictionary(pdict);
//
//	atp_message amsg;
//	amsg.type = ATP_MESSAGE_TYPE_SERVER_TRADE_RSP;
//	amsg.param1 = (void*)rd;
//	mdpt_->dispatch_message(amsg);
//
//	return NAUT_AT_S_OK;
//}

int trade_unit::process_withdraw_result(CThostFtdcOrderField *pOrder,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    base::dictionary* pdict = new base::dictionary();
    base::dictionary& dict = *pdict;

    dict["cmd"] = ATPM_CMD_WITHDRAW_RESPONSE;
    dict["userid"] = userid_;
    dict["account"] = account_;
    dict["broker"] = broker_;
    if (bIsLast) {
        dict["result_complete"] = "1";
    } else {
        dict["result_complete"] = "0";
    }
    dict["systemno"] = pOrder->RequestID;
    dict["entrustno"] = pOrder->OrderSysID;
    dict["localno"] = pOrder->OrderRef;
//	dict["withdrawno"] = pOrder->OrderActionRef;
    dict["broker"] = pOrder->BrokerID;
    dict["code"] = pOrder->InstrumentID;

    if (IsErrorRspInfo(pRspInfo)) {
        dict["at_error"] = (long) pRspInfo->ErrorID;
    } else {
        dict["at_error"] = (long) AT_ERROR_NONE;

        switch (pOrder->CombOffsetFlag[0]) {
            case '0':
                dict["entrust_type"] = (long) 1;
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
                dict["entrust_type"] = (long) 2;
                break;
            default:
                TRACE_WARNING(AT_TRACE_TAG, "entrust_type: %c is not right", pOrder->CombOffsetFlag[0]);
                break;
        }

        if (pOrder->Direction == THOST_FTDC_D_Buy) {
            dict["direction"] = (long) 1;
        } else if(pOrder->Direction == THOST_FTDC_D_Sell) {
            dict["direction"] = (long) 2;
        }

        dict["entrust_amount"] = pOrder->VolumeTotalOriginal;
        dict["entrust_price"] = pOrder->LimitPrice;
        dict["deal_amount"] = pOrder->VolumeTraded;
        dict["withdraw_amount"] = pOrder->VolumeTotal;

        char datebuf[11];
        memset(datebuf, 0, sizeof(datebuf));
        ChangeDateFormat(datebuf, pOrder->GTDDate);
        dict["withdraw_date"] = datebuf;
        dict["withdraw_time"] = pOrder->CancelTime;
    }

    ref_dictionary* rd = new ref_dictionary(pdict);

    atp_message amsg;
    amsg.type = ATP_MESSAGE_TYPE_SERVER_TRADE_RSP;
    amsg.param1 = (void*) rd;
    mdpt_->dispatch_message(amsg);

    return NAUT_AT_S_OK;
}

int trade_unit::process_qryaccount_result(
        CThostFtdcTradingAccountField *pTradingAccount,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    base::dictionary* pdict = new base::dictionary();
    base::dictionary& dict = *pdict;

    dict["cmd"] = ATPM_CMD_RSP_ACCOUNT;
    if (bIsLast) {
        dict["result_complete"] = "1";
    } else {
        dict["result_complete"] = "0";
    }

    if (IsErrorRspInfo(pRspInfo)) {
        dict["userid"] = userid_;
        dict["account"] = account_;
        dict["broker"] = broker_;
        dict["at_error"] = (long) pRspInfo->ErrorID;
    } else {
        dict["userid"] = userid_;
        dict["account"] = account_;
        dict["broker"] = broker_;
        dict["pre_canuse"] = pTradingAccount->Available;
        dict["pre_equity"] = pTradingAccount->PreBalance;
        // dict["pre_amount"] = msg.GetString(ZD_OLD_AMOUNT);
        dict["in_money"] = pTradingAccount->Deposit;
        dict["out_money"] = pTradingAccount->Withdraw;
        dict["equity"] = pTradingAccount->Balance;
        //add by cwm
        // dict["today_amount"] = msg.GetString(ZD_TODAY_AMOUNT);
        dict["canuse_fund"] = pTradingAccount->Available;
        dict["deposit"] = pTradingAccount->Deposit;
        dict["frozen_deposit"] = pTradingAccount->FrozenMargin;
        dict["fee"] = pTradingAccount->Commission;
        dict["expired_profit"] = pTradingAccount->CloseProfit;
        //		dict["net_profit"] = msg.GetString(ZD_NET_PROFIT);
        //		dict["profit_rate"] = msg.GetString(ZD_PROFIT_RATE);
        //		dict["risk_rate"] = msg.GetString(ZD_RISK_RATE);
        //		dict["currencyno"] = msg.GetString(ZD_CURRENCY_NO);
        //		dict["currency_rate"] = msg.GetString(ZD_CURRENCY_RATE);
        //		dict["unexpired_profit"] = msg.GetString(ZD_UNEXPIRED_PROFIT);
        //		dict["unaccount_profit"] = msg.GetString(ZD_UNACCOUNT_PROFIT);
        //		dict["keep_deposit"] = msg.GetString(ZD_KEEP_DEPOSIT);
        //		dict["royalty"] = msg.GetString(ZD_ROYALTY);
        dict["at_error"] = (long) AT_ERROR_CTP_NONE;
    }

    ref_dictionary* rd = new ref_dictionary(pdict);

    atp_message amsg;
    amsg.type = ATP_MESSAGE_TYPE_SERVER_QUERY_RSP;
    amsg.param1 = (void*) rd;
    mdpt_->dispatch_message(amsg);

    return NAUT_AT_S_OK;
}

int trade_unit::process_qrydeal_result(CThostFtdcTradeField *pTrade,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    base::dictionary* pdict = new base::dictionary();
    base::dictionary& dict = *pdict;

    dict["cmd"] = ATPM_CMD_RSP_DEAL;
    if (bIsLast) {
        dict["result_complete"] = "1";
    } else {
        dict["result_complete"] = "0";
    }
    if (IsErrorRspInfo(pRspInfo)) {
        dict["userid"] = userid_;
        dict["account"] = account_;
        dict["broker"] = broker_;
        dict["at_error"] = (long) pRspInfo->ErrorID;
    } else {
        dict["userid"] = userid_;
        dict["account"] = account_;
        dict["broker"] = broker_;
        dict["entrustno"] = pTrade->OrderRef;
        dict["dealno"] = pTrade->TradeID;
        dict["systemno"] = (long) nRequestID;
        dict["localno"] = pTrade->OrderLocalID;
        dict["excode"] = pTrade->ExchangeID;
        dict["code"] = pTrade->InstrumentID;

        switch (pTrade ->OffsetFlag) {
             case '0':
                 dict["entrust_type"] = (long) 1;
                 break;
             case '1':
             case '2':
             case '3':
             case '4':
             case '5':
             case '6':
                 dict["entrust_type"] = (long) 2;
                 break;
             default:
                 TRACE_WARNING(AT_TRACE_TAG, "entrust_type: %c is not right", pTrade->OffsetFlag);
                 break;
         }

         if (pTrade->Direction == THOST_FTDC_D_Buy) {
             dict["direction"] = (long) 1;
         } else if(pTrade->Direction == THOST_FTDC_D_Sell) {
             dict["direction"] = (long) 2;
         }

        dict["deal_amount"] = pTrade->Volume;
        dict["deal_price"] = pTrade->Price;

        char datebuf[11];
        memset(datebuf, 0, sizeof(datebuf));
        ChangeDateFormat(datebuf, pTrade->TradeDate);
        dict["deal_date"] = datebuf;
        dict["deal_time"] = pTrade->TradeTime;
        // dict["fee"] = pTrade->;
        // dict["at_code"] = pTrade->;
        dict["change_date"] = pTrade->TradingDay;
//		dict["deal_type"] = pTrade->;
        dict["at_error"] = (long) AT_ERROR_CTP_NONE;
    }

    ref_dictionary* rd = new ref_dictionary(pdict);

    atp_message amsg;
    amsg.type = ATP_MESSAGE_TYPE_SERVER_QUERY_RSP;
    amsg.param1 = (void*) rd;
    mdpt_->dispatch_message(amsg);

    return NAUT_AT_S_OK;
}

int trade_unit::process_qryopeniner_result(
        CThostFtdcInvestorPositionField *pInvestorPosition,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    base::dictionary* pdict = new base::dictionary();
    base::dictionary& dict = *pdict;

    dict["cmd"] = ATPM_CMD_RSP_OPENINTER;
    if (bIsLast) {
        dict["result_complete"] = "1";
    } else {
        dict["result_complete"] = "0";
    }
    if (IsErrorRspInfo(pRspInfo)) {
        dict["userid"] = userid_;
        dict["account"] = account_;
        dict["broker"] = broker_;
        dict["at_error"] = (long) pRspInfo->ErrorID;
    } else {
        dict["userid"] = userid_;
        dict["account"] = account_;
        dict["broker"] = broker_;
        // dict["currency_type"] = pInvestorPosition->;
        // dict["com_type"] = pInvestorPosition->;
        dict["code"] = pInvestorPosition->InstrumentID;

        char datebuf[11];
        memset(datebuf, 0, sizeof(datebuf));
        ChangeDateFormat(datebuf, pInvestorPosition->TradingDay);
        dict["deal_date"] = datebuf;
        dict["dealno"] = pInvestorPosition->SettlementID;
        if (pInvestorPosition->PosiDirection == THOST_FTDC_D_Buy) {
            dict["bs"] = (long) 1;
        } else {
            dict["bs"] = (long) 2;
        }

        dict["open_amount"] = pInvestorPosition->OpenVolume;
        // dict["init_open_price"] = pInvestorPosition->;
        dict["open_price"] = pInvestorPosition->OpenCost;
        // dict["open_status"] = pInvestorPosition->;
        // dict["change_date"] = pInvestorPosition->TradingDay;
        dict["at_error"] = (long) AT_ERROR_CTP_NONE;
    }
    TRACE_DEBUG(AT_TRACE_TAG, "openiner: %s", dict.to_string().c_str());

    ref_dictionary* rd = new ref_dictionary(pdict);

    atp_message amsg;
    amsg.type = ATP_MESSAGE_TYPE_SERVER_QUERY_RSP;
    amsg.param1 = (void*) rd;
    mdpt_->dispatch_message(amsg);

    return NAUT_AT_S_OK;
}

int trade_unit::process_qryentrust_result(CThostFtdcOrderField *pOrder,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    base::dictionary* pdict = new base::dictionary();
    base::dictionary& dict = *pdict;

    dict["cmd"] = ATPM_CMD_RSP_ENTRUST;
//    if (bIsLast) {
    dict["result_complete"] = "1";
//    } else {
//        dict["result_complete"] = "0";
//    }
    if (IsErrorRspInfo(pRspInfo)) {
        dict["userid"] = userid_;
        dict["account"] = account_;
        dict["broker"] = broker_;
        dict["at_error"] = (long) pRspInfo->ErrorID;
    } else {
        dict["userid"] = userid_;
        dict["account"] = account_;
        dict["broker"] = broker_;
        dict["systemno"] = pOrder->RequestID;
        dict["localno"] = pOrder->OrderRef;
        dict["entrustno"] = pOrder->OrderSysID;
        dict["excode"] = pOrder->ExchangeID;
        dict["code"] = pOrder->InstrumentID;

        switch (pOrder->CombOffsetFlag[0]) {
             case '0':
                 dict["entrust_type"] = (long) 1;
                 break;
             case '1':
             case '2':
             case '3':
             case '4':
             case '5':
             case '6':
                 dict["entrust_type"] = (long) 2;
                 break;
             default:
                 TRACE_WARNING(AT_TRACE_TAG, "entrust_type: %c is not right", pOrder->CombOffsetFlag[0]);
                 break;
         }

         if (pOrder->Direction == THOST_FTDC_D_Buy) {
             dict["direction"] = (long) 1;
         } else if(pOrder->Direction == THOST_FTDC_D_Sell) {
             dict["direction"] = (long) 2;
         }

        dict["entrust_amount"] = pOrder->VolumeTotalOriginal;
        // dict["entrust_price"] = pOrder->LimitPrice;
        dict["deal_amount"] = pOrder->VolumeTraded;
        dict["deal_price"] = pOrder->LimitPrice;

        // price_type: 价格类别（1 limit order 2 market order）
        if (pOrder->OrderPriceType == THOST_FTDC_OPT_LimitPrice) {
            dict["price_type"] = (long) 1;
        } else {
            dict["price_type"] = (long) 2;
        }

        char datebuf[11];
        memset(datebuf, 0, sizeof(datebuf));
        ChangeDateFormat(datebuf, pOrder->GTDDate);

        dict["withdraw_date"] = datebuf;
        dict["withdraw_time"] = pOrder->CancelTime;
        dict["at_error"] = (long) AT_ERROR_CTP_NONE;
        // （1: 订单已接收，并开始处理 2: 订单已委托 3: 撤单指令已接收，并开始处理 4: 订单有成交
        //  5: 订单全部成交 6: 订单撤单成功（可能存在部分成交部分撤单）7: 订单失败, 8:未知, 9:尚未触发, 10:已触发）
        switch (pOrder->OrderStatus) {
            ///全部成交
            case THOST_FTDC_OST_AllTraded:
                dict["order_status"] = (long) 5;
                break;
                ///部分成交还在队列中
            case THOST_FTDC_OST_PartTradedQueueing:
                dict["order_status"] = (long) 4;
                break;
                ///部分成交不在队列中
            case THOST_FTDC_OST_PartTradedNotQueueing:
                dict["order_status"] = (long) 6;
                break;
                ///未成交还在队列中
            case THOST_FTDC_OST_NoTradeQueueing:
                dict["order_status"] = (long) 2;
                break;
                ///未成交不在队列中
            case THOST_FTDC_OST_NoTradeNotQueueing:
                dict["order_status"] = (long) 1;
                break;
                ///撤单
            case THOST_FTDC_OST_Canceled:
                dict["order_status"] = (long) 6;
                break;
                ///未知
            case THOST_FTDC_OST_Unknown:
                dict["order_status"] = (long) 8;
                break;
                ///尚未触发
            case THOST_FTDC_OST_NotTouched:
                dict["order_status"] = (long) 9;
                break;
                ///已触发
            case THOST_FTDC_OST_Touched:
                dict["order_status"] = (long) 10;
                break;
        }

        memset(datebuf, 0, sizeof(datebuf));
        ChangeDateFormat(datebuf, pOrder->InsertDate);
        dict["entrust_date"] = datebuf;
        dict["entrust_time"] = pOrder->InsertTime;
        dict["at_error"] = (long) AT_ERROR_CTP_NONE;
    }

    ref_dictionary* rd = new ref_dictionary(pdict);

    atp_message amsg;
    amsg.type = ATP_MESSAGE_TYPE_SERVER_QUERY_RSP;
    amsg.param1 = (void*) rd;
    mdpt_->dispatch_message(amsg);

    return NAUT_AT_S_OK;
}
//
//int trade_unit::process_requestid_result(CShZdMessage& msg)
//{
//	if (strcmp(msg.GetString(ZD_INFO_OVER), "1") == 0) {
//		return NAUT_AT_S_OK;
//	}
//
//	base::dictionary* pdict = new base::dictionary();
//	base::dictionary& dict = *pdict;
//
//	dict["cmd"] = ATPM_CMD_SYSTEMNO_RESPONSE;
//	dict["userid"] = userid_;
//	dict["account"] = account_;
//	dict["systemno"] = msg.GetString(ZD_SYSTEM_NO);
//	dict["localno"] = msg.GetString(ZD_LOCAL_NO);
//	dict["excode"] = msg.GetString(ZD_EXCHANGE_CODE);
//	dict["code"] = msg.GetString(ZD_CODE);
//	dict["bs"] = msg.GetString(ZD_BUY_SALE);
//	dict["entrust_amount"] = msg.GetString(ZD_ORDER_NUMBER);
//	dict["entrust_price"] = msg.GetString(ZD_ORDER_PRICE);
//	dict["at_error"] = msg.GetString(ZD_ERROR_CODE);
//	if (dict["at_error"].string_value() == CTP_S_EMPTY) {
//		dict["at_error"] = "";
//	}
//
//	ref_dictionary* rd = new ref_dictionary(pdict);
//
//	atp_message amsg;
//	amsg.type = ATP_MESSAGE_TYPE_SERVER_TRADE_RSP;
//	amsg.param1 = (void*)rd;
//	mdpt_->dispatch_message(amsg);
//
//	return NAUT_AT_S_OK;
//}

int trade_unit::GetRequestId()
{
    return base_fetch_and_inc(&m_RequestId_);
}

///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
void trade_unit::OnFrontConnected()
{
    m_last_respond = base::util::clock();
    TRACE_SYSTEM(AT_TRACE_TAG,
            "broker_account:%s_%s connection to server sucessfully",
            params_.broker.c_str(), params_.account.c_str());
    connected_ = true;
    m_conn_state = cs_connected;

    if (started_) {
        state_event_->set();
    }
    if (!logined_) {
        login();
    }
}

///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
///@param nReason 错误原因
///        0x1001 网络读失败
///        0x1002 网络写失败
///        0x2001 接收心跳超时
///        0x2002 发送心跳失败
///        0x2003 收到错误报文
void trade_unit::OnFrontDisconnected(int nReason)
{
    m_last_respond = base::util::clock();
    connected_ = false;
    m_conn_state = cs_disconnected;
    std::string errmsg("");
    switch (nReason) {
        case 0x1001:
            errmsg = "网络读失败";
            break;
        case 0x1002:
            errmsg = "网络写失败";
            break;
        case 0x2001:
            errmsg = "接收心跳超时";
            break;
        case 0x2002:
            errmsg = "发送心跳失败";
            break;
        case 0x2003:
            errmsg = "收到错误报文";
            break;
    }
    TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_CTPTRADE_NOT_CONNECTED,
            "connection to server failed '%d', will retry later, '%s'", nReason,
            errmsg.c_str());
}

///心跳超时警告。当长时间未收到报文时，该方法被调用。
///@param nTimeLapse 距离上次接收报文的时间
void trade_unit::OnHeartBeatWarning(int nTimeLapse)
{
    connected_ = false;
	TRACE_WARNING(AT_TRACE_TAG, "tu(%s)  the heartbeat is over time (%ds) from last one", userid_.c_str(), nTimeLapse);
}

///客户端认证响应
void trade_unit::OnRspAuthenticate(
        CThostFtdcRspAuthenticateField *pRspAuthenticateField,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s)  pRspAuthenticateField:%p, pRspInfo:%p, nRequestID:%d, bIsLast:%d",
                        userid_.c_str(), pRspAuthenticateField, pRspInfo, nRequestID, bIsLast);
    if (!pRspAuthenticateField) {
        return;
    }
}

///登录请求响应
void trade_unit::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    m_last_respond = base::util::clock();
    if (!pRspUserLogin) {
        return;
    }
    /* if receive login response, indicate that it is connected */
    m_last_respond = base::util::clock();
    if (!IsErrorRspInfo(pRspInfo)) {
        logined_ = true;

        assert(state_event_ != NULL);
        state_event_->set();
        m_FrontID_ = pRspUserLogin->FrontID;
        m_SessionID_ = pRspUserLogin->SessionID;
        m_MaxOrderRef_ = pRspUserLogin->MaxOrderRef;

        TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): login successfully~~~ m_FrontID_:%d, m_SessionID_:%d, m_MaxOrderRef_:%s",
                userid_.c_str(), m_FrontID_, m_SessionID_, m_MaxOrderRef_.c_str());

        CThostFtdcSettlementInfoConfirmField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, broker_.c_str());
        strcpy(req.InvestorID, account_.c_str());
        while (true)
        {
            m_last_respond = base::util::clock();
            int iResult = trader_->ReqSettlementInfoConfirm(&req, GetRequestId());

            if (!iResult)
            {
                TRACE_SYSTEM(AT_TRACE_TAG, "ReqSettlementInfoConfirm result: %d \n", iResult);
                break;
            }
            else
            {
                TRACE_SYSTEM(AT_TRACE_TAG, "ReqSettlementInfoConfirm result: %d \n", iResult);
                base::util::sleep(1000);
            }

        } // while
    } else {
        TRACE_SYSTEM(AT_TRACE_TAG,
                "tu(%s): receive login response, userid: %s, BrokerID:%s ,errormsg: %s",
                userid_.c_str(), pRspUserLogin->UserID, pRspUserLogin->BrokerID,
                get_response_error_msg(pRspInfo->ErrorID).c_str());
    }

}

///登出请求响应
void trade_unit::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    m_last_respond = base::util::clock();
    connected_ = false;
    m_conn_state = cs_disconnected;

    if (!pUserLogout) {
        return;
    } else {
        TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): logout successfully~~~",
                userid_.c_str());
    }

    if (!IsErrorRspInfo(pRspInfo)) {
        logined_ = false;

        assert(state_event_ != NULL);
        state_event_->reset();

        TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): logout successfully~~~",
                userid_.c_str());
    } else {
        TRACE_SYSTEM(AT_TRACE_TAG,
                "tu(%s): receive logout response, userid: %s, BrokerID:%s ,errormsg: %s",
                userid_.c_str(), pUserLogout->UserID, pUserLogout->BrokerID,
                get_response_error_msg(pRspInfo->ErrorID).c_str());
    }
}

///用户口令更新请求响应
void trade_unit::OnRspUserPasswordUpdate(
        CThostFtdcUserPasswordUpdateField *pUserPasswordUpdate,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pUserPasswordUpdate:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pUserPasswordUpdate, pRspInfo, nRequestID, bIsLast);
    if (!pUserPasswordUpdate) {
        return;
    }
}

///资金账户口令更新请求响应
void trade_unit::OnRspTradingAccountPasswordUpdate(
        CThostFtdcTradingAccountPasswordUpdateField *pTradingAccountPasswordUpdate,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pUserPasswordUpdate:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pTradingAccountPasswordUpdate, pRspInfo, nRequestID, bIsLast);
    if (!pTradingAccountPasswordUpdate) {
        return;
    }
}

///报单录入请求响应
void trade_unit::OnRspOrderInsert(CThostFtdcInputOrderField *pInputOrder,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    m_last_respond = base::util::clock();
    TRACE_SYSTEM(AT_TRACE_TAG, "pInputOrder:%p, pRspInfo:%p, nRequestID:%d, bIsLast:%d", pInputOrder, pRspInfo, nRequestID, bIsLast);
    if (!pInputOrder) {
        return;
    }

    CThostFtdcOrderField rspOrder;
    memset(&rspOrder, 0, sizeof(rspOrder));
    strcpy(rspOrder.BrokerID, pInputOrder->BrokerID);
    strcpy(rspOrder.BusinessUnit, pInputOrder->BusinessUnit);
    strcpy(rspOrder.CombHedgeFlag, pInputOrder->CombHedgeFlag);
    strcpy(rspOrder.CombOffsetFlag, pInputOrder->CombOffsetFlag);
    strcpy(rspOrder.GTDDate, pInputOrder->GTDDate);
    strcpy(rspOrder.InstrumentID, pInputOrder->InstrumentID);
    strcpy(rspOrder.InvestorID, pInputOrder->InvestorID);
    strcpy(rspOrder.OrderRef, pInputOrder->OrderRef);
    strcpy(rspOrder.UserID, pInputOrder->UserID);

    rspOrder.ContingentCondition = pInputOrder->ContingentCondition;
    rspOrder.Direction = pInputOrder->Direction;
    rspOrder.ForceCloseReason = pInputOrder->ForceCloseReason;
    rspOrder.IsAutoSuspend = pInputOrder->IsAutoSuspend;
    rspOrder.IsSwapOrder = pInputOrder->IsSwapOrder;
    rspOrder.LimitPrice = pInputOrder->LimitPrice;
    rspOrder.MinVolume = pInputOrder->MinVolume;
    rspOrder.OrderPriceType = pInputOrder->OrderPriceType;
    rspOrder.RequestID = pInputOrder->RequestID;
    rspOrder.StopPrice = pInputOrder->StopPrice;
    rspOrder.TimeCondition = pInputOrder->TimeCondition;
    rspOrder.UserForceClose = pInputOrder->UserForceClose;
    rspOrder.VolumeCondition = pInputOrder->VolumeCondition;
    rspOrder.VolumeTotalOriginal = pInputOrder->VolumeTotalOriginal;

    process_entrust_result(&rspOrder, pRspInfo, nRequestID, bIsLast);
}

///预埋单录入请求响应
void trade_unit::OnRspParkedOrderInsert(
        CThostFtdcParkedOrderField *pParkedOrder,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    if (!pParkedOrder) {
        return;
    }
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pParkedOrder:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pParkedOrder, pRspInfo, nRequestID, bIsLast);
}

///预埋撤单录入请求响应
void trade_unit::OnRspParkedOrderAction(
        CThostFtdcParkedOrderActionField *pParkedOrderAction,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pParkedOrderAction:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pParkedOrderAction, pRspInfo, nRequestID, bIsLast);
    if (!pParkedOrderAction) {
        return;
    }
}

///报单操作请求响应
void trade_unit::OnRspOrderAction(
        CThostFtdcInputOrderActionField *pInputOrderAction,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    m_last_respond = base::util::clock();
    if (!pInputOrderAction) {
        return;
    }
    CThostFtdcOrderField rspOrder;
    memset(&rspOrder, 0, sizeof(rspOrder));
    strcpy(rspOrder.BrokerID, pInputOrderAction->BrokerID);
    strcpy(rspOrder.InvestorID, pInputOrderAction->InvestorID);
    strcpy(rspOrder.OrderRef, pInputOrderAction->OrderRef);
    rspOrder.RequestID = pInputOrderAction->RequestID;
    rspOrder.FrontID = pInputOrderAction->FrontID;
    rspOrder.SessionID = pInputOrderAction->SessionID;
    strcpy(rspOrder.ExchangeID, pInputOrderAction->ExchangeID);
    strcpy(rspOrder.OrderSysID, pInputOrderAction->OrderSysID);
    rspOrder.LimitPrice = pInputOrderAction->LimitPrice;
    rspOrder.VolumeTotal = pInputOrderAction->VolumeChange;
    strcpy(rspOrder.UserID, pInputOrderAction->UserID);
    strcpy(rspOrder.InstrumentID, pInputOrderAction->InstrumentID);

    process_withdraw_result(&rspOrder, pRspInfo, nRequestID, bIsLast);
}

///查询最大报单数量响应
void trade_unit::OnRspQueryMaxOrderVolume(
        CThostFtdcQueryMaxOrderVolumeField *pQueryMaxOrderVolume,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pQueryMaxOrderVolume:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pQueryMaxOrderVolume, pRspInfo, nRequestID, bIsLast);
    if (!pQueryMaxOrderVolume) {
        return;
    }
}

///投资者结算结果确认响应
void trade_unit::OnRspSettlementInfoConfirm(
        CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pSettlementInfoConfirm:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pSettlementInfoConfirm, pRspInfo, nRequestID, bIsLast);
    if (!pSettlementInfoConfirm) {
        return;
    }

    if (bIsLast && !IsErrorRspInfo(pRspInfo))
    {
        m_stockhold_old_buy_ = 0;
        m_stockhold_old_sell_ = 0;

        CThostFtdcQryInvestorPositionField QryInvestorPosition;
        memset(&QryInvestorPosition, 0, sizeof(QryInvestorPosition));
        strcpy(QryInvestorPosition.InvestorID, userid_.c_str());
        strcpy(QryInvestorPosition.BrokerID, broker_.c_str());

        int iResult = 0;
        while (true)
        {
            iResult = trader_->ReqQryInvestorPosition(&QryInvestorPosition, GetRequestId());

            if (!iResult)
            {
                TRACE_SYSTEM(AT_TRACE_TAG, "ReqQryInvestorPosition result: %d \n", iResult);
                break;
            }
            else
            {
                TRACE_SYSTEM(AT_TRACE_TAG, "ReqQryInvestorPosition result: %d \n", iResult);
                base::util::sleep(1000);
            }

        } // while
    }
}

///删除预埋单响应
void trade_unit::OnRspRemoveParkedOrder(
        CThostFtdcRemoveParkedOrderField *pRemoveParkedOrder,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pRemoveParkedOrder:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pRemoveParkedOrder, pRspInfo, nRequestID, bIsLast);
    if (!pRemoveParkedOrder) {
        return;
    }
}

///删除预埋撤单响应
void trade_unit::OnRspRemoveParkedOrderAction(
        CThostFtdcRemoveParkedOrderActionField *pRemoveParkedOrderAction,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pRemoveParkedOrderAction:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pRemoveParkedOrderAction, pRspInfo, nRequestID, bIsLast);
    if (!pRemoveParkedOrderAction) {
        return;
    }
}

///执行宣告录入请求响应
void trade_unit::OnRspExecOrderInsert(
        CThostFtdcInputExecOrderField *pInputExecOrder,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInputExecOrder:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pInputExecOrder, pRspInfo, nRequestID, bIsLast);
    if (!pInputExecOrder) {
        return;
    }
}

///执行宣告操作请求响应
void trade_unit::OnRspExecOrderAction(
        CThostFtdcInputExecOrderActionField *pInputExecOrderAction,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInputExecOrderAction:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pInputExecOrderAction, pRspInfo, nRequestID, bIsLast);
    if (!pInputExecOrderAction) {
        return;
    }
}

///询价录入请求响应
void trade_unit::OnRspForQuoteInsert(
        CThostFtdcInputForQuoteField *pInputForQuote,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInputForQuote:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pInputForQuote, pRspInfo, nRequestID, bIsLast);
    if (!pInputForQuote) {
        return;
    }
}

///报价录入请求响应
void trade_unit::OnRspQuoteInsert(CThostFtdcInputQuoteField *pInputQuote,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInputQuote:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pInputQuote, pRspInfo, nRequestID, bIsLast);
    if (!pInputQuote) {
        return;
    }
}

///报价操作请求响应
void trade_unit::OnRspQuoteAction(
        CThostFtdcInputQuoteActionField *pInputQuoteAction,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInputQuoteAction:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pInputQuoteAction, pRspInfo, nRequestID, bIsLast);
    if (!pInputQuoteAction) {
        return;
    }
}

///申请组合录入请求响应
void trade_unit::OnRspCombActionInsert(
        CThostFtdcInputCombActionField *pInputCombAction,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInputCombAction:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pInputCombAction, pRspInfo, nRequestID, bIsLast);
    if (!pInputCombAction) {
        return;
    }
}

///请求查询报单响应
void trade_unit::OnRspQryOrder(CThostFtdcOrderField *pOrder,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    m_last_respond = base::util::clock();
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pOrder:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pOrder, pRspInfo, nRequestID, bIsLast);
    if (!pOrder) {
        return;
    }
    process_qryentrust_result(pOrder, pRspInfo, nRequestID, bIsLast);
}

///请求查询成交响应
void trade_unit::OnRspQryTrade(CThostFtdcTradeField *pTrade,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pTrade:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pTrade, pRspInfo, nRequestID, bIsLast);
    if (!pTrade) {
        return;
    }
    process_qrydeal_result(pTrade, pRspInfo, nRequestID, bIsLast);
}

///请求查询投资者持仓响应
void trade_unit::OnRspQryInvestorPosition(
        CThostFtdcInvestorPositionField *pInvestorPosition,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    m_last_respond = base::util::clock();
    if (!pInvestorPosition) {
        return;
    }
    if (pInvestorPosition->PosiDirection == '2'){
        if (pInvestorPosition->YdPosition > 0 && pInvestorPosition->Position > 0)
            m_stockhold_old_buy_ += pInvestorPosition->Position;
    }else if (pInvestorPosition->PosiDirection == '3'){
        if (pInvestorPosition->YdPosition > 0 && pInvestorPosition->Position > 0)
            m_stockhold_old_sell_ += pInvestorPosition->Position;
    }

    process_qryopeniner_result(pInvestorPosition, pRspInfo, nRequestID,
            bIsLast);
}

///请求查询资金账户响应
void trade_unit::OnRspQryTradingAccount(
        CThostFtdcTradingAccountField *pTradingAccount,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
	 TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): recv rsp_account nRequestID = %d",
	                    userid_.c_str(), nRequestID);
    m_last_respond = base::util::clock();
    if (!pTradingAccount) {
        return;
    }
    process_qryaccount_result(pTradingAccount, pRspInfo, nRequestID, bIsLast);
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): finish rsp_account nRequestID = %d",
    	                    userid_.c_str(), nRequestID);
    m_last_qryacc_finish = true;
}

///请求查询投资者响应
void trade_unit::OnRspQryInvestor(CThostFtdcInvestorField *pInvestor,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInvestor:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pInvestor, pRspInfo, nRequestID, bIsLast);
    if (!pInvestor) {
        return;
    }
}

///请求查询交易编码响应
void trade_unit::OnRspQryTradingCode(CThostFtdcTradingCodeField *pTradingCode,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pTradingCode:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pTradingCode, pRspInfo, nRequestID, bIsLast);
    if (!pTradingCode) {
        return;
    }
}

///请求查询合约保证金率响应
void trade_unit::OnRspQryInstrumentMarginRate(
        CThostFtdcInstrumentMarginRateField *pInstrumentMarginRate,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInstrumentMarginRate:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pInstrumentMarginRate, pRspInfo, nRequestID, bIsLast);
    if (!pInstrumentMarginRate) {
        return;
    }
}

///请求查询合约手续费率响应
void trade_unit::OnRspQryInstrumentCommissionRate(
        CThostFtdcInstrumentCommissionRateField *pInstrumentCommissionRate,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInstrumentCommissionRate:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pInstrumentCommissionRate, pRspInfo, nRequestID, bIsLast);
    if (!pInstrumentCommissionRate) {
        return;
    }
}

///请求查询交易所响应
void trade_unit::OnRspQryExchange(CThostFtdcExchangeField *pExchange,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pExchange:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pExchange, pRspInfo, nRequestID, bIsLast);
    if (!pExchange) {
        return;
    }
}

///请求查询产品响应
void trade_unit::OnRspQryProduct(CThostFtdcProductField *pProduct,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pProduct:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pProduct, pRspInfo, nRequestID, bIsLast);
    if (!pProduct) {
        return;
    }
}

///请求查询合约响应
void trade_unit::OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInstrument:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pInstrument, pRspInfo, nRequestID, bIsLast);
    if (!pInstrument) {
        return;
    }
}

///请求查询行情响应
void trade_unit::OnRspQryDepthMarketData(
        CThostFtdcDepthMarketDataField *pDepthMarketData,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    m_last_respond = base::util::clock();
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pDepthMarketData:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pDepthMarketData, pRspInfo, nRequestID, bIsLast);
    if (!pDepthMarketData) {
        return;
    }
}

///请求查询投资者结算结果响应
void trade_unit::OnRspQrySettlementInfo(
        CThostFtdcSettlementInfoField *pSettlementInfo,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pSettlementInfo:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pSettlementInfo, pRspInfo, nRequestID, bIsLast);
    if (!pSettlementInfo) {
        return;
    }
}

///请求查询转帐银行响应
void trade_unit::OnRspQryTransferBank(
        CThostFtdcTransferBankField *pTransferBank,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pTransferBank:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pTransferBank, pRspInfo, nRequestID, bIsLast);
    if (!pTransferBank) {
        return;
    }
}

///请求查询投资者持仓明细响应
void trade_unit::OnRspQryInvestorPositionDetail(
        CThostFtdcInvestorPositionDetailField *pInvestorPositionDetail,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInvestorPositionDetail:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pInvestorPositionDetail, pRspInfo, nRequestID, bIsLast);
    if (!pInvestorPositionDetail) {
        return;
    }
}

///请求查询客户通知响应
void trade_unit::OnRspQryNotice(CThostFtdcNoticeField *pNotice,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pNotice:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pNotice, pRspInfo, nRequestID, bIsLast);
    if (!pNotice) {
        return;
    }
}

///请求查询结算信息确认响应
void trade_unit::OnRspQrySettlementInfoConfirm(
        CThostFtdcSettlementInfoConfirmField *pSettlementInfoConfirm,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pSettlementInfoConfirm:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pSettlementInfoConfirm, pRspInfo, nRequestID, bIsLast);
    if (!pSettlementInfoConfirm) {
        return;
    }
}

///请求查询投资者持仓明细响应
void trade_unit::OnRspQryInvestorPositionCombineDetail(
        CThostFtdcInvestorPositionCombineDetailField *pInvestorPositionCombineDetail,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInvestorPositionCombineDetail:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pInvestorPositionCombineDetail, pRspInfo, nRequestID, bIsLast);
    if (!pInvestorPositionCombineDetail) {
        return;
    }
}

///查询保证金监管系统经纪公司资金账户密钥响应
void trade_unit::OnRspQryCFMMCTradingAccountKey(
        CThostFtdcCFMMCTradingAccountKeyField *pCFMMCTradingAccountKey,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pCFMMCTradingAccountKey:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pCFMMCTradingAccountKey, pRspInfo, nRequestID, bIsLast);
    if (!pCFMMCTradingAccountKey) {
        return;
    }
}

///请求查询仓单折抵信息响应
void trade_unit::OnRspQryEWarrantOffset(
        CThostFtdcEWarrantOffsetField *pEWarrantOffset,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pEWarrantOffset:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pEWarrantOffset, pRspInfo, nRequestID, bIsLast);
    if (!pEWarrantOffset) {
        return;
    }
}

///请求查询投资者品种/跨品种保证金响应
void trade_unit::OnRspQryInvestorProductGroupMargin(
        CThostFtdcInvestorProductGroupMarginField *pInvestorProductGroupMargin,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInvestorProductGroupMargin:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pInvestorProductGroupMargin, pRspInfo, nRequestID, bIsLast);
    if (!pInvestorProductGroupMargin) {
        return;
    }
}

///请求查询交易所保证金率响应
void trade_unit::OnRspQryExchangeMarginRate(
        CThostFtdcExchangeMarginRateField *pExchangeMarginRate,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pExchangeMarginRate:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pExchangeMarginRate, pRspInfo, nRequestID, bIsLast);
    if (!pExchangeMarginRate) {
        return;
    }
}

///请求查询交易所调整保证金率响应
void trade_unit::OnRspQryExchangeMarginRateAdjust(
        CThostFtdcExchangeMarginRateAdjustField *pExchangeMarginRateAdjust,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pExchangeMarginRateAdjust:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pExchangeMarginRateAdjust, pRspInfo, nRequestID, bIsLast);
    if (!pExchangeMarginRateAdjust) {
        return;
    }
}

///请求查询汇率响应
void trade_unit::OnRspQryExchangeRate(
        CThostFtdcExchangeRateField *pExchangeRate,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pExchangeRate:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pExchangeRate, pRspInfo, nRequestID, bIsLast);
    if (!pExchangeRate) {
        return;
    }
}

///请求查询二级代理操作员银期权限响应
void trade_unit::OnRspQrySecAgentACIDMap(
        CThostFtdcSecAgentACIDMapField *pSecAgentACIDMap,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pSecAgentACIDMap:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pSecAgentACIDMap, pRspInfo, nRequestID, bIsLast);
    if (!pSecAgentACIDMap) {
        return;
    }
}

///请求查询产品组
void trade_unit::OnRspQryProductGroup(
        CThostFtdcProductGroupField *pProductGroup,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pProductGroup:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pProductGroup, pRspInfo, nRequestID, bIsLast);
    if (!pProductGroup) {
        return;
    }
}

///请求查询报单手续费响应
void trade_unit::OnRspQryInstrumentOrderCommRate(
        CThostFtdcInstrumentOrderCommRateField *pInstrumentOrderCommRate,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInstrumentOrderCommRate:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pInstrumentOrderCommRate, pRspInfo, nRequestID, bIsLast);
    if (!pInstrumentOrderCommRate) {
        return;
    }
}

///请求查询期权交易成本响应
void trade_unit::OnRspQryOptionInstrTradeCost(
        CThostFtdcOptionInstrTradeCostField *pOptionInstrTradeCost,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pOptionInstrTradeCost:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pOptionInstrTradeCost, pRspInfo, nRequestID, bIsLast);
    if (!pOptionInstrTradeCost) {
        return;
    }
}

///请求查询期权合约手续费响应
void trade_unit::OnRspQryOptionInstrCommRate(
        CThostFtdcOptionInstrCommRateField *pOptionInstrCommRate,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pOptionInstrCommRate:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pOptionInstrCommRate, pRspInfo, nRequestID, bIsLast);
    if (!pOptionInstrCommRate) {
        return;
    }
}

///请求查询执行宣告响应
void trade_unit::OnRspQryExecOrder(CThostFtdcExecOrderField *pExecOrder,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pExecOrder:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pExecOrder, pRspInfo, nRequestID, bIsLast);
    if (!pExecOrder) {
        return;
    }
}

///请求查询询价响应
void trade_unit::OnRspQryForQuote(CThostFtdcForQuoteField *pForQuote,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pForQuote:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pForQuote, pRspInfo, nRequestID, bIsLast);
    if (!pForQuote) {
        return;
    }
}

///请求查询报价响应
void trade_unit::OnRspQryQuote(CThostFtdcQuoteField *pQuote,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pQuote:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pQuote, pRspInfo, nRequestID, bIsLast);
    if (!pQuote) {
        return;
    }
}

///请求查询组合合约安全系数响应
void trade_unit::OnRspQryCombInstrumentGuard(
        CThostFtdcCombInstrumentGuardField *pCombInstrumentGuard,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pCombInstrumentGuard:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pCombInstrumentGuard, pRspInfo, nRequestID, bIsLast);
    if (!pCombInstrumentGuard) {
        return;
    }
}

///请求查询申请组合响应
void trade_unit::OnRspQryCombAction(CThostFtdcCombActionField *pCombAction,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pCombAction:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pCombAction, pRspInfo, nRequestID, bIsLast);
    if (!pCombAction) {
        return;
    }
}

///请求查询转帐流水响应
void trade_unit::OnRspQryTransferSerial(
        CThostFtdcTransferSerialField *pTransferSerial,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pTransferSerial:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pTransferSerial, pRspInfo, nRequestID, bIsLast);
    if (!pTransferSerial) {
        return;
    }
}

///请求查询银期签约关系响应
void trade_unit::OnRspQryAccountregister(
        CThostFtdcAccountregisterField *pAccountregister,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pAccountregister:%p, pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pAccountregister, pRspInfo, nRequestID, bIsLast);
    if (!pAccountregister) {
        return;
    }
}

///错误应答
void trade_unit::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID,
        bool bIsLast)
{
    connected_ = false;
    TRACE_ERROR(AT_TRACE_TAG,NAUT_AT_E_UNKNOWN_TRADE_CMD, "tu(%s): pRspInfo:%p, nRequestID: %d, bIsLast:%d",
                    userid_.c_str(), pRspInfo, nRequestID, bIsLast);
}

///报单通知
void trade_unit::OnRtnOrder(CThostFtdcOrderField *pOrder)
{
    m_last_respond = base::util::clock();
    if (!pOrder) {
        return;
    }

    switch (map_int_req_fun_[pOrder->RequestID]) {
        case PROCESS_ENTRUST_RESULT_REQUESTID:
            TRACE_DEBUG(AT_TRACE_TAG, "pOrder:%p, nRequestID:%d", pOrder, pOrder->RequestID);
            process_entrust_result(pOrder, NULL, pOrder->RequestID, true);
            break;
        case PROCESS_WITHDRAW_RESULT_REQUESTID:
            process_withdraw_result(pOrder, NULL, pOrder->RequestID, true);
            break;
        case PROCESS_QRYENTRUST_RESULT_REQUESTID:
            process_qryentrust_result(pOrder, NULL, pOrder->RequestID, true);
            break;
        default:
            TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ORDER_NOT_EXIST,
                    "unknown userid:'%s' systemno : '%s' ", userid_.c_str(),
                    pOrder->RequestID);
            break;
    }

}

///成交通知
void trade_unit::OnRtnTrade(CThostFtdcTradeField *pTrade)
{
    m_last_respond = base::util::clock();
    if (!pTrade) {
        return;
    }
    TRACE_DEBUG(AT_TRACE_TAG, "pTrade:%p", pTrade);
    process_entrust_deal_result(pTrade);
}

///报单录入错误回报
void trade_unit::OnErrRtnOrderInsert(CThostFtdcInputOrderField *pInputOrder,
        CThostFtdcRspInfoField *pRspInfo)
{
    m_last_respond = base::util::clock();
    TRACE_SYSTEM(AT_TRACE_TAG, "pInputOrder:%p, pRspInfo:%p", pInputOrder, pRspInfo);
    if (!pInputOrder) {
        return;
    }
    TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ORDER_NOT_EXIST,
            "received error: '%d', errormsg, '%s'", pRspInfo->ErrorID,
            get_response_error_msg(pRspInfo->ErrorID).c_str());

    CThostFtdcOrderField rspOrder;
    memset(&rspOrder, 0, sizeof(rspOrder));
    strcpy(rspOrder.BrokerID, pInputOrder->BrokerID);
    strcpy(rspOrder.BusinessUnit, pInputOrder->BusinessUnit);
    strcpy(rspOrder.CombHedgeFlag, pInputOrder->CombHedgeFlag);
    strcpy(rspOrder.CombOffsetFlag, pInputOrder->CombOffsetFlag);
    strcpy(rspOrder.GTDDate, pInputOrder->GTDDate);
    strcpy(rspOrder.InstrumentID, pInputOrder->InstrumentID);
    strcpy(rspOrder.InvestorID, pInputOrder->InvestorID);
    strcpy(rspOrder.OrderRef, pInputOrder->OrderRef);
    strcpy(rspOrder.UserID, pInputOrder->UserID);

    rspOrder.ContingentCondition = pInputOrder->ContingentCondition;
    rspOrder.Direction = pInputOrder->Direction;
    rspOrder.ForceCloseReason = pInputOrder->ForceCloseReason;
    rspOrder.IsAutoSuspend = pInputOrder->IsAutoSuspend;
    rspOrder.IsSwapOrder = pInputOrder->IsSwapOrder;
    rspOrder.LimitPrice = pInputOrder->LimitPrice;
    rspOrder.MinVolume = pInputOrder->MinVolume;
    rspOrder.OrderPriceType = pInputOrder->OrderPriceType;
    rspOrder.RequestID = pInputOrder->RequestID;
    rspOrder.StopPrice = pInputOrder->StopPrice;
    rspOrder.TimeCondition = pInputOrder->TimeCondition;
    rspOrder.UserForceClose = pInputOrder->UserForceClose;
    rspOrder.VolumeCondition = pInputOrder->VolumeCondition;
    rspOrder.VolumeTotalOriginal = pInputOrder->VolumeTotalOriginal;

    process_entrust_result(&rspOrder, pRspInfo, 0, true);
}

///报单操作错误回报
void trade_unit::OnErrRtnOrderAction(CThostFtdcOrderActionField *pOrderAction,
        CThostFtdcRspInfoField *pRspInfo)
{
    if (!pOrderAction) {
        return;
    }
    TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_ORDER_NOT_EXIST,
            "received error: '%d', errormsg, '%s'", pRspInfo->ErrorID,
            get_response_error_msg(pRspInfo->ErrorID).c_str());
}

///合约交易状态通知
void trade_unit::OnRtnInstrumentStatus(
        CThostFtdcInstrumentStatusField *pInstrumentStatus)
{
    m_last_respond = base::util::clock();
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pInstrumentStatus:%p",
                    userid_.c_str(), pInstrumentStatus);

    m_last_respond = base::util::clock();
}

///交易通知
void trade_unit::OnRtnTradingNotice(
        CThostFtdcTradingNoticeInfoField *pTradingNoticeInfo)
{
    m_last_respond = base::util::clock();
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pTradingNoticeInfo:%p",
                    userid_.c_str(), pTradingNoticeInfo);
    m_last_respond = base::util::clock();
}

///提示条件单校验错误
void trade_unit::OnRtnErrorConditionalOrder(
        CThostFtdcErrorConditionalOrderField *pErrorConditionalOrder)
{
    m_last_respond = base::util::clock();
    TRACE_ERROR(AT_TRACE_TAG,NAUT_AT_E_UNKNOWN_TRADE_CMD, "tu(%s): pErrorConditionalOrder:%p ",
                    userid_.c_str(), pErrorConditionalOrder);
}

///执行宣告通知
void trade_unit::OnRtnExecOrder(CThostFtdcExecOrderField *pExecOrder)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pExecOrder:%p",
                    userid_.c_str(), pExecOrder);
}

///执行宣告录入错误回报
void trade_unit::OnErrRtnExecOrderInsert(
        CThostFtdcInputExecOrderField *pInputExecOrder,
        CThostFtdcRspInfoField *pRspInfo)
{
    TRACE_ERROR(AT_TRACE_TAG,NAUT_AT_E_UNKNOWN_TRADE_CMD, "tu(%s): pInputExecOrder:%p , pRspInfo:%p",
                    userid_.c_str(), pInputExecOrder, pRspInfo);
}

///执行宣告操作错误回报
void trade_unit::OnErrRtnExecOrderAction(
        CThostFtdcExecOrderActionField *pExecOrderAction,
        CThostFtdcRspInfoField *pRspInfo)
{
    TRACE_ERROR(AT_TRACE_TAG,NAUT_AT_E_UNKNOWN_TRADE_CMD, "tu(%s): pExecOrderAction:%p , pRspInfo:%p",
                    userid_.c_str(), pExecOrderAction, pRspInfo);
}

///询价录入错误回报
void trade_unit::OnErrRtnForQuoteInsert(
        CThostFtdcInputForQuoteField *pInputForQuote,
        CThostFtdcRspInfoField *pRspInfo)
{
    TRACE_ERROR(AT_TRACE_TAG,NAUT_AT_E_UNKNOWN_TRADE_CMD, "tu(%s): pInputForQuote:%p , pRspInfo:%p",
                    userid_.c_str(), pInputForQuote, pRspInfo);
}

///报价通知
void trade_unit::OnRtnQuote(CThostFtdcQuoteField *pQuote)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pQuote:%p",
                    userid_.c_str(), pQuote);
}

///报价录入错误回报
void trade_unit::OnErrRtnQuoteInsert(CThostFtdcInputQuoteField *pInputQuote,
        CThostFtdcRspInfoField *pRspInfo)
{
    TRACE_ERROR(AT_TRACE_TAG,NAUT_AT_E_UNKNOWN_TRADE_CMD, "tu(%s): pInputQuote:%p , pRspInfo:%p",
                    userid_.c_str(), pInputQuote, pRspInfo);
}

///报价操作错误回报
void trade_unit::OnErrRtnQuoteAction(CThostFtdcQuoteActionField *pQuoteAction,
        CThostFtdcRspInfoField *pRspInfo)
{
    TRACE_ERROR(AT_TRACE_TAG,NAUT_AT_E_UNKNOWN_TRADE_CMD, "tu(%s): pQuoteAction:%p , pRspInfo:%p",
                    userid_.c_str(), pQuoteAction, pRspInfo);
}

///询价通知
void trade_unit::OnRtnForQuoteRsp(CThostFtdcForQuoteRspField *pForQuoteRsp)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pForQuoteRsp:%p",
                    userid_.c_str(), pForQuoteRsp);
}

///保证金监控中心用户令牌
void trade_unit::OnRtnCFMMCTradingAccountToken(
        CThostFtdcCFMMCTradingAccountTokenField *pCFMMCTradingAccountToken)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pCFMMCTradingAccountToken:%p",
                    userid_.c_str(), pCFMMCTradingAccountToken);
}

///申请组合通知
void trade_unit::OnRtnCombAction(CThostFtdcCombActionField *pCombAction)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pCombAction:%p",
                    userid_.c_str(), pCombAction);
}

///申请组合录入错误回报
void trade_unit::OnErrRtnCombActionInsert(
        CThostFtdcInputCombActionField *pInputCombAction,
        CThostFtdcRspInfoField *pRspInfo)
{
    TRACE_ERROR(AT_TRACE_TAG,NAUT_AT_E_UNKNOWN_TRADE_CMD, "tu(%s): pInputCombAction:%p , pRspInfo:%p",
                    userid_.c_str(), pInputCombAction, pRspInfo);
}

///请求查询签约银行响应
void trade_unit::OnRspQryContractBank(
        CThostFtdcContractBankField *pContractBank,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pContractBank:%p, pRspInfo:%p, nRequestID:%d, bIsLast:%d",
                    userid_.c_str(), pContractBank, pRspInfo, nRequestID, bIsLast);
}

///请求查询预埋单响应
void trade_unit::OnRspQryParkedOrder(CThostFtdcParkedOrderField *pParkedOrder,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pParkedOrder:%p, pRspInfo:%p, nRequestID:%d, bIsLast:%d",
                    userid_.c_str(), pParkedOrder, pRspInfo, nRequestID, bIsLast);
}

///请求查询预埋撤单响应
void trade_unit::OnRspQryParkedOrderAction(
        CThostFtdcParkedOrderActionField *pParkedOrderAction,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pParkedOrderAction:%p, pRspInfo:%p, nRequestID:%d, bIsLast:%d",
                    userid_.c_str(), pParkedOrderAction, pRspInfo, nRequestID, bIsLast);
}

///请求查询交易通知响应
void trade_unit::OnRspQryTradingNotice(
        CThostFtdcTradingNoticeField *pTradingNotice,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pTradingNotice:%p, pRspInfo:%p, nRequestID:%d, bIsLast:%d",
                    userid_.c_str(), pTradingNotice, pRspInfo, nRequestID, bIsLast);
}

///请求查询经纪公司交易参数响应
void trade_unit::OnRspQryBrokerTradingParams(
        CThostFtdcBrokerTradingParamsField *pBrokerTradingParams,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pBrokerTradingParams:%p, pRspInfo:%p, nRequestID:%d, bIsLast:%d",
                    userid_.c_str(), pBrokerTradingParams, pRspInfo, nRequestID, bIsLast);
}

///请求查询经纪公司交易算法响应
void trade_unit::OnRspQryBrokerTradingAlgos(
        CThostFtdcBrokerTradingAlgosField *pBrokerTradingAlgos,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pBrokerTradingAlgos:%p, pRspInfo:%p, nRequestID:%d, bIsLast:%d",
                    userid_.c_str(), pBrokerTradingAlgos, pRspInfo, nRequestID, bIsLast);
}

///请求查询监控中心用户令牌
void trade_unit::OnRspQueryCFMMCTradingAccountToken(
        CThostFtdcQueryCFMMCTradingAccountTokenField *pQueryCFMMCTradingAccountToken,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pQueryCFMMCTradingAccountToken:%p, pRspInfo:%p, nRequestID:%d, bIsLast:%d",
                    userid_.c_str(), pQueryCFMMCTradingAccountToken, pRspInfo, nRequestID, bIsLast);
}

///银行发起银行资金转期货通知
void trade_unit::OnRtnFromBankToFutureByBank(
        CThostFtdcRspTransferField *pRspTransfer)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pRspTransfer:%p",
                    userid_.c_str(), pRspTransfer);
}

///银行发起期货资金转银行通知
void trade_unit::OnRtnFromFutureToBankByBank(
        CThostFtdcRspTransferField *pRspTransfer)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pRspTransfer:%p",
                    userid_.c_str(), pRspTransfer);
}

///银行发起冲正银行转期货通知
void trade_unit::OnRtnRepealFromBankToFutureByBank(
        CThostFtdcRspRepealField *pRspRepeal)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pRspRepeal:%p",
                    userid_.c_str(), pRspRepeal);
}

///银行发起冲正期货转银行通知
void trade_unit::OnRtnRepealFromFutureToBankByBank(
        CThostFtdcRspRepealField *pRspRepeal)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pRspRepeal:%p",
                    userid_.c_str(), pRspRepeal);
}

///期货发起银行资金转期货通知
void trade_unit::OnRtnFromBankToFutureByFuture(
        CThostFtdcRspTransferField *pRspTransfer)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pRspTransfer:%p",
                    userid_.c_str(), pRspTransfer);
}

///期货发起期货资金转银行通知
void trade_unit::OnRtnFromFutureToBankByFuture(
        CThostFtdcRspTransferField *pRspTransfer)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pRspTransfer:%p",
                    userid_.c_str(), pRspTransfer);
}

///系统运行时期货端手工发起冲正银行转期货请求，银行处理完毕后报盘发回的通知
void trade_unit::OnRtnRepealFromBankToFutureByFutureManual(
        CThostFtdcRspRepealField *pRspRepeal)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pRspRepeal:%p",
                    userid_.c_str(), pRspRepeal);
}

///系统运行时期货端手工发起冲正期货转银行请求，银行处理完毕后报盘发回的通知
void trade_unit::OnRtnRepealFromFutureToBankByFutureManual(
        CThostFtdcRspRepealField *pRspRepeal)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pRspRepeal:%p",
                    userid_.c_str(), pRspRepeal);
}

///期货发起查询银行余额通知
void trade_unit::OnRtnQueryBankBalanceByFuture(
        CThostFtdcNotifyQueryAccountField *pNotifyQueryAccount)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pNotifyQueryAccount:%p",
                    userid_.c_str(), pNotifyQueryAccount);
}

///期货发起银行资金转期货错误回报
void trade_unit::OnErrRtnBankToFutureByFuture(
        CThostFtdcReqTransferField *pReqTransfer,
        CThostFtdcRspInfoField *pRspInfo)
{
    TRACE_ERROR(AT_TRACE_TAG,NAUT_AT_E_UNKNOWN_TRADE_CMD, "tu(%s): pReqTransfer:%p , pRspInfo:%p",
                    userid_.c_str(), pReqTransfer, pRspInfo);
}

///期货发起期货资金转银行错误回报
void trade_unit::OnErrRtnFutureToBankByFuture(
        CThostFtdcReqTransferField *pReqTransfer,
        CThostFtdcRspInfoField *pRspInfo)
{
    TRACE_ERROR(AT_TRACE_TAG,NAUT_AT_E_UNKNOWN_TRADE_CMD, "tu(%s): pReqTransfer:%p , pRspInfo:%p",
                    userid_.c_str(), pReqTransfer, pRspInfo);
}

///系统运行时期货端手工发起冲正银行转期货错误回报
void trade_unit::OnErrRtnRepealBankToFutureByFutureManual(
        CThostFtdcReqRepealField *pReqRepeal, CThostFtdcRspInfoField *pRspInfo)
{
    TRACE_ERROR(AT_TRACE_TAG,NAUT_AT_E_UNKNOWN_TRADE_CMD, "tu(%s): pReqRepeal:%p , pRspInfo:%p",
                    userid_.c_str(), pReqRepeal, pRspInfo);
}

///系统运行时期货端手工发起冲正期货转银行错误回报
void trade_unit::OnErrRtnRepealFutureToBankByFutureManual(
        CThostFtdcReqRepealField *pReqRepeal, CThostFtdcRspInfoField *pRspInfo)
{
    TRACE_ERROR(AT_TRACE_TAG,NAUT_AT_E_UNKNOWN_TRADE_CMD, "tu(%s): pReqRepeal:%p , pRspInfo:%p",
                    userid_.c_str(), pReqRepeal, pRspInfo);
}

///期货发起查询银行余额错误回报
void trade_unit::OnErrRtnQueryBankBalanceByFuture(
        CThostFtdcReqQueryAccountField *pReqQueryAccount,
        CThostFtdcRspInfoField *pRspInfo)
{
    TRACE_ERROR(AT_TRACE_TAG,NAUT_AT_E_UNKNOWN_TRADE_CMD, "tu(%s): pReqQueryAccount:%p , pRspInfo:%p",
                    userid_.c_str(), pReqQueryAccount, pRspInfo);
}

///期货发起冲正银行转期货请求，银行处理完毕后报盘发回的通知
void trade_unit::OnRtnRepealFromBankToFutureByFuture(
        CThostFtdcRspRepealField *pRspRepeal)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pRspRepeal:%p",
                    userid_.c_str(), pRspRepeal);
}

///期货发起冲正期货转银行请求，银行处理完毕后报盘发回的通知
void trade_unit::OnRtnRepealFromFutureToBankByFuture(
        CThostFtdcRspRepealField *pRspRepeal)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pRspRepeal:%p",
                    userid_.c_str(), pRspRepeal);
}

///期货发起银行资金转期货应答
void trade_unit::OnRspFromBankToFutureByFuture(
        CThostFtdcReqTransferField *pReqTransfer,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pReqTransfer:%p, pRspInfo:%p, nRequestID:%d, bIsLast:%d",
                    userid_.c_str(), pReqTransfer, pRspInfo, nRequestID, bIsLast);
}

///期货发起期货资金转银行应答
void trade_unit::OnRspFromFutureToBankByFuture(
        CThostFtdcReqTransferField *pReqTransfer,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pReqTransfer:%p, pRspInfo:%p, nRequestID:%d, bIsLast:%d",
                    userid_.c_str(), pReqTransfer, pRspInfo, nRequestID, bIsLast);
}

///期货发起查询银行余额应答
void trade_unit::OnRspQueryBankAccountMoneyByFuture(
        CThostFtdcReqQueryAccountField *pReqQueryAccount,
        CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pReqQueryAccount:%p, pRspInfo:%p, nRequestID:%d, bIsLast:%d",
                    userid_.c_str(), pReqQueryAccount, pRspInfo, nRequestID, bIsLast);
}

///银行发起银期开户通知
void trade_unit::OnRtnOpenAccountByBank(
        CThostFtdcOpenAccountField *pOpenAccount)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pOpenAccount:%p ",
                    userid_.c_str(), pOpenAccount);
}

///银行发起银期销户通知
void trade_unit::OnRtnCancelAccountByBank(
        CThostFtdcCancelAccountField *pCancelAccount)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pCancelAccount:%p ",
                    userid_.c_str(), pCancelAccount);
}

///银行发起变更银行账号通知
void trade_unit::OnRtnChangeAccountByBank(
        CThostFtdcChangeAccountField *pChangeAccount)
{
    TRACE_SYSTEM(AT_TRACE_TAG, "tu(%s): pChangeAccount:%p ",
                    userid_.c_str(), pChangeAccount);
}

std::string trade_unit::GetRetErrorMsg(int errorcode)
{
    std::string errmsg("");
    switch (errorcode) {
        case -1:
            errmsg = "因网络原因发送失败";
            break;
        case -2:
            errmsg = "未处理请求队列总数量超限";
            break;
        case -3:
            errmsg = "每秒发送请求数量超限";
            break;
        default:
            errmsg = "错误未定义";
            break;

    }
    return errmsg;
}

char* trade_unit::ChangeDateFormat(char* outdate, char* inputdate)
{
    if (outdate == NULL || inputdate == NULL) {
        return NULL;
    }
    for (int i = 0, j = 0; i < 11; i++) {
        if (i == 4 || i == 7) {
            outdate[i] = '-';
            continue;
        }
        outdate[i] = inputdate[j];
        j++;
    }
    return outdate;
}

bool trade_unit::IsErrorRspInfo(CThostFtdcRspInfoField *pRspInfo)
{
    return ((pRspInfo) && (pRspInfo->ErrorID != AT_ERROR_CTP_NONE));
}

int trade_unit::process_entrust_result(const base::dictionary& dict)
{
    base::dictionary* pdict = new base::dictionary(dict);
    base::dictionary& indict = *pdict;
    indict["cmd"] = ATPM_CMD_ENTRUST_RESPONSE;
    indict["result_complete"] = "1";
    ref_dictionary* rd = new ref_dictionary(pdict);
    atp_message amsg;
    amsg.type = ATP_MESSAGE_TYPE_SERVER_TRADE_RSP;
    amsg.param1 = (void*) rd;
    mdpt_->dispatch_message(amsg);

    return NAUT_AT_S_OK;
}

// entrustno <-> OrderSysID
// dealno    <-> TradeID
// localno   <-> OrderRef

int trade_unit::process_entrust_deal_result(const base::dictionary& dict)
{
    base::dictionary* pdict = new base::dictionary(dict);
    base::dictionary& indict = *pdict;

    indict["cmd"] = ATPM_CMD_DEAL;

    ref_dictionary* rd = new ref_dictionary(pdict);

    atp_message amsg;
    amsg.type = ATP_MESSAGE_TYPE_SERVER_TRADE_RSP;
    amsg.param1 = (void*) rd;
    mdpt_->dispatch_message(amsg);

    return NAUT_AT_S_OK;
}

int trade_unit::process_withdraw_result(const base::dictionary& dict)
{
    base::dictionary* pdict = new base::dictionary(dict);
    base::dictionary& indict = *pdict;

    indict["cmd"] = ATPM_CMD_WITHDRAW_RESPONSE;

    ref_dictionary* rd = new ref_dictionary(pdict);

    atp_message amsg;
    amsg.type = ATP_MESSAGE_TYPE_SERVER_TRADE_RSP;
    amsg.param1 = (void*) rd;
    mdpt_->dispatch_message(amsg);

    return NAUT_AT_S_OK;
}

int trade_unit::process_qryaccount_result(const base::dictionary& dict)
{
    base::dictionary* pdict = new base::dictionary(dict);
    base::dictionary& indict = *pdict;

    indict["cmd"] = ATPM_CMD_RSP_ACCOUNT;
    indict["result_complete"] = "1";

    ref_dictionary* rd = new ref_dictionary(pdict);

    atp_message amsg;
    amsg.type = ATP_MESSAGE_TYPE_SERVER_QUERY_RSP;
    amsg.param1 = (void*) rd;
    mdpt_->dispatch_message(amsg);

    return NAUT_AT_S_OK;
}

int trade_unit::process_qrydeal_result(const base::dictionary& dict)
{
    base::dictionary* pdict = new base::dictionary(dict);
    base::dictionary& indict = *pdict;

    indict["cmd"] = ATPM_CMD_RSP_DEAL;
    indict["result_complete"] = "1";

    ref_dictionary* rd = new ref_dictionary(pdict);

    atp_message amsg;
    amsg.type = ATP_MESSAGE_TYPE_SERVER_QUERY_RSP;
    amsg.param1 = (void*) rd;
    mdpt_->dispatch_message(amsg);

    return NAUT_AT_S_OK;
}

int trade_unit::process_qryopeniner_result(const base::dictionary& dict)
{
    base::dictionary* pdict = new base::dictionary(dict);
    base::dictionary& indict = *pdict;

    indict["cmd"] = ATPM_CMD_RSP_OPENINTER;
    indict["result_complete"] = "1";
    TRACE_DEBUG(AT_TRACE_TAG, "openiner: %s", dict.to_string().c_str());

    ref_dictionary* rd = new ref_dictionary(pdict);

    atp_message amsg;
    amsg.type = ATP_MESSAGE_TYPE_SERVER_QUERY_RSP;
    amsg.param1 = (void*) rd;
    mdpt_->dispatch_message(amsg);

    return NAUT_AT_S_OK;
}

int trade_unit::process_qryentrust_result(const base::dictionary& dict)
{
    base::dictionary* pdict = new base::dictionary(dict);
    base::dictionary& indict = *pdict;

    indict["cmd"] = ATPM_CMD_RSP_ENTRUST;
    indict["result_complete"] = "1";

    ref_dictionary* rd = new ref_dictionary(pdict);

    atp_message amsg;
    amsg.type = ATP_MESSAGE_TYPE_SERVER_QUERY_RSP;
    amsg.param1 = (void*) rd;
    mdpt_->dispatch_message(amsg);

    return NAUT_AT_S_OK;
}

}
