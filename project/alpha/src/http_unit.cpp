/*****************************************************************************
 returns_report_manager Copyright (c) 2016. All Rights Reserved.

 FileName: http_unit.cpp
 Version: 1.0
 Date: 2016.03.39

 History:
 david wang     2016.03.29   1.0     create
 ******************************************************************************/

#include "http_unit.h"
#include "base/dictionary.h"
#include "base/dtrans.h"
namespace serverframe
{

http_unit::http_unit()
    : processor_base()
    , server_name_("")
    , msg_event_(NULL)
    , started_(false)
{
}

http_unit::~http_unit()
{
    stop();
}

int http_unit::start(const char* server_name)
{
    int ret = NAUT_S_OK;

    if (started_) {
        CHECK_LABEL(ret, end);
    }

    server_name_ = server_name;
    m_http_client_.set_body_post_cb(post_msg_cb);
    m_http_client_.set_body_get_cb(get_msg_cb);

    ret = start_internal();
    CHECK_LABEL(ret, end);

end:
    if (BSUCCEEDED(ret)) {
        started_ = true;
    }
    else {
        stop();
    }
    return ret;
}

void http_unit::stop()
{
    started_ = false;
    stop_internal();
}

int http_unit::start_internal()
{
    int ret = NAUT_S_OK;
    msg_event_ = new base::event();
    processor_base::start();

    return ret;
}

int http_unit::stop_internal()
{
    int ret = NAUT_S_OK;

    processor_base::stop();
    if (msg_event_ != NULL) {
        delete msg_event_;
        msg_event_ = NULL;
    }

    return ret;
}

void http_unit::run()
{
    atp_message msg;
    while (is_running_)
    {
        int ret = get(msg);
        if (ret != 0) {
            msg_event_->reset();
            msg_event_->wait(20);
            continue;
        }

        ref_dictionary* rd = (ref_dictionary*)msg.param1;
        base::dictionary* dict = rd->get();

        string httpstr = base::djson::dict2str(dict);
        httpstr.erase(httpstr.end() - 1);
        char* poststr = (char*) msg.param2;

        TRACE_SYSTEM(MODULE_NAME, "begin http_unit post or get packages: %s, httpstr:[ %s ], poststr: [ %s ]",
                dict->to_string().c_str(), httpstr.c_str(), poststr);

        if(msg.type == HTTP_CLIENT_MSG_TYPE_POST) { //used
            m_http_client_.http_request_post_msg(poststr, httpstr.c_str());
        } else if(msg.type == HTTP_CLIENT_MSG_TYPE_GET) { //unused
            m_http_client_.http_request_get_msg(poststr, httpstr.c_str());
        }

        TRACE_SYSTEM(MODULE_NAME, "end http_unit post or get packages: %s, httpstr:[ %s ], poststr: [ %s ]",
                dict->to_string().c_str(), httpstr.c_str(), poststr);

        delete[] poststr;
        poststr = NULL;
        rd->release();
    }
}

void http_unit::post(const atp_message& msg)
{
    msg_queue_->push(msg);
    if (started_) {
        assert(msg_event_ != NULL);
        msg_event_->set();
    }
}

void http_unit::post_msg_cb(char* des, size_t size)
{
    TRACE_SYSTEM(MODULE_NAME, "recevie post_cb msg : %s ", des);
}

void http_unit::get_msg_cb(char* des, size_t size)
{
    TRACE_SYSTEM(MODULE_NAME, "recevie get_cb msg : %s ", des);
}

}


