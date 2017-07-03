/*****************************************************************************
 http_manager Copyright (c) 2016. All Rights Reserved.

 FileName: http_processor.cpp
 Version: 1.0
 Date: 2016.03.23

 History:
 cwm     2016.03.23   1.0     Create
 ******************************************************************************/

#include "http_processor.h"
#include "product_business_deal.h"
#include "project_server.h"
#include "base/dictionary.h"
#include "base/dtrans.h"
#include <string>
#include "http_deal.h"
namespace serverframe {

bool g_is_service_available = true;

#define check_fail_handler(msg) \
    string json = "{\"result\":\"N\",\"msg\":\"";\
        json += msg;\
        json += "\"}";\
    evbuffer_add_printf(evb, "%s", json.c_str());\


void send_document_cb(struct evhttp_request* req, void* arg)
{
    int ret = 0;

    struct evkeyvalq *output_Headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_Headers, "Content-Type", "text/html; charset=utf-8");
    evhttp_add_header(output_Headers, "Connection", "keep-alive");
    evhttp_add_header(output_Headers, "Cache-Control", "no-cache");

    struct evhttp_connection *conn = evhttp_request_get_connection(req);
    http_deal deal;
    char *addr;
    ev_uint16_t port;
    evhttp_connection_get_peer(conn, &addr, &port);

    const char *mncg_query_part = evhttp_request_get_uri(req);
    const char *mncg_query_part1 = NULL;
    struct evkeyvalq mncg_http_query;
    memset(&mncg_http_query, 0, sizeof(mncg_http_query));
    if (strncmp(mncg_query_part, "/?", 2) == 0) {
        mncg_query_part1 = mncg_query_part + 2;
    }
    struct evbuffer *evb = evbuffer_new();
    evhttp_parse_query_str(mncg_query_part1, &mncg_http_query);

    const char *mncg_input_opt = evhttp_find_header(&mncg_http_query, "opt");
    if (mncg_input_opt == NULL || strlen(mncg_input_opt) <= 4) {
        check_fail_handler("error opt");
        goto deal_error;
    }
    if (strcmp(mncg_input_opt, "service_onoff") == 0) {
        ret = deal.service_onoff(output_Headers, mncg_http_query, evb);
        if (ret != 0) {
            goto deal_error;
        }
        else {
            evhttp_send_reply(req, 200, "OK", evb);
            goto done;
        }
    }
    if (!g_is_service_available) {
        //evbuffer_add_printf(evb, "%s", errno_transfer(ERROR_SYSTEM_UNAVAILABLE));
        //set_response_code(ERROR_SYSTEM_UNAVAILABLE);
        //set_response_result("N");
        //set_response_msg(errno_transfer(ERROR_SYSTEM_UNAVAILABLE));
        check_fail_handler("erro service unavailable");
        goto deal_error;
    }

    base::dictionary *ptr_dic = NULL;
    int buffer_data_len = EVBUFFER_LENGTH(req->input_buffer);
    if (buffer_data_len <= 10) {
        check_fail_handler("erro post illegal data len");
        goto deal_error;
    }
    else {
        char *post_data = (char *)(EVBUFFER_DATA(req->input_buffer));
        char *package = new char[buffer_data_len + 1];
        memcpy(package, post_data, buffer_data_len);
        package[buffer_data_len] = '\0';

        TRACE_INFO(MODULE_NAME, "http post_data:%s", package);

        ptr_dic = base::djson::str2dict(package);
        delete[] package;
        package = NULL;

        if (ptr_dic == NULL) {
            check_fail_handler("erro post illegal data");
            goto deal_error;
        }
    }

    if (strcmp(mncg_input_opt, "get_dce_instrument") == 0) {
        ret = deal.account_fund_query(output_Headers, ptr_dic, evb);
        if (ret != 0) {
            goto deal_error;
        }
    }
    else {
        check_fail_handler("error opt type");
        goto deal_error;
    }

    evhttp_send_reply(req, 200, "OK", evb);
    goto done;

deal_error:
    TRACE_ERROR(MODULE_NAME, MANAGER_E_SERVER_NOTIFY_DATA_INVALID,
        "request(%s)deal failed\n", mncg_query_part);
    evhttp_send_reply(req, 200, "OK", evb);
    goto done;
done:
    evhttp_clear_headers(&mncg_http_query);
    if (evb) {
        evbuffer_free(evb);
    }
}
base::multi_thread_httpserver* http_processor::http_ = NULL;

http_processor::http_processor() {

}

http_processor::~http_processor() {
    stop();
}

int http_processor::start() {
    int ret = NAUT_S_OK;
    string msg = "";
    http_ = new base::multi_thread_httpserver();
    ret = http_->init(get_project_server().get_process_param().serverinfo_.ip, 
        get_project_server().get_process_param().serverinfo_.port, send_document_cb, msg);
    if (ret != 0) {
        TRACE_ERROR(MODULE_NAME, MANAGER_E_SERVER_NOTIFY_DATA_INVALID,
            "http init error %s", msg.c_str());
        return -1;
    }

    http_->run();

    return 0;
}

int http_processor::stop() {

    if (http_) {
        delete http_;
        http_ = NULL;
    }

    return 0;
}

}
