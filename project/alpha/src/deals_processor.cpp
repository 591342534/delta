/*****************************************************************************
 returns_report_manager Copyright (c) 2016. All Rights Reserved.

 FileName: deals_processor.cpp
 Version: 1.0
 Date: 2016.03.15

 History:
 cwm     2016.03.15   1.0     Create
 david wang     2016.03.16   1.0     modfiy
 ******************************************************************************/

#include "deals_processor.h"
#include "product_business_deal.h"
#include "process.h"
#include "base/util.h"
#include "base/dtrans.h"
#include <sstream>

namespace serverframe
{

deals_processor::deals_processor()
        : processor_base()
        , server_name_("")
        , msg_event_(NULL)
        , started_(false)
{
}

deals_processor::~deals_processor()
{

}

int deals_processor::start(const char* server_name)
{
    int ret = NAUT_S_OK;

    if (started_) {
        CHECK_LABEL(ret, end);
    }

    server_name_ = server_name;

    ret = start_internal();
    CHECK_LABEL(ret, end);

end:
    if (BSUCCEEDED(ret)) {
        started_ = true;
    } else {
        stop();
    }
    return ret;
}

void deals_processor::stop()
{
    started_ = false;
    stop_internal();
}

int deals_processor::start_internal()
{
    int ret = NAUT_S_OK;

    msg_event_ = new base::event();

    /* start process thread */
    processor_base::start();

    return ret;
}

int deals_processor::stop_internal()
{
    int ret = NAUT_S_OK;
    /* stop process thread */
    processor_base::stop();

    if (msg_event_ != NULL) {
        delete msg_event_;
        msg_event_ = NULL;
    }

    return ret;
}

void deals_processor::run()
{
    atp_message msg;
    while (is_running_) {
        int ret = get(msg);
        if (ret != 0) {
            msg_event_->reset();
            msg_event_->wait(20);
            continue;
        }

        ref_dictionary* rd = (ref_dictionary*) msg.param1;
        base::dictionary* dict = rd->get();

        TRACE_SYSTEM(MODULE_NAME, "deals_processor packages: %s",
                dict->to_string().c_str());

        if ((*dict)["cmd"].string_value() == ATPM_CMD_ENTRUST_RESULT) { //used
            entrust_result_deal(*dict);
        } else if ((*dict)["cmd"].string_value() == ATPM_CMD_DEAL_RESULT) { //used
            deal_result_deal(*dict);
        }else if ((*dict)["cmd"].string_value() == ATPM_CMD_WITHDRAW_RESULT) { //used
            withdraw_result_deal(*dict);
        }
        rd->release();
    }
}

void deals_processor::post(const atp_message& msg)
{
    msg_queue_->push(msg);
    if (started_) {
        assert(msg_event_ != NULL);
        msg_event_->set();
    }
}

int deals_processor::entrust_result_deal(base::dictionary &dict)
{
    int ret = NAUT_S_OK;

    LABEL_SCOPE_START;

    LABEL_SCOPE_END;
end:
    return ret;
}

int deals_processor::deal_result_deal(base::dictionary &dict)
{
    int ret = NAUT_S_OK;

    LABEL_SCOPE_START;

    LABEL_SCOPE_END;
end:
    return ret;
}

int deals_processor::withdraw_result_deal(base::dictionary &dict)
{
    int ret = NAUT_S_OK;

    LABEL_SCOPE_START;


    LABEL_SCOPE_END;
end:
    return ret;
}

}

