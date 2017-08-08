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
#include "base/file.h"
#include "database/unidbpool.h"

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


    /* start processor thread */
    processor_base::start();

    LABEL_SCOPE_END;

    end:
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


        rd->release();
    }
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

}
