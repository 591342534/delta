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

namespace serverframe {

inline void send_error_response(struct evbuffer *evb, const char* addr,
        const char * request, int err_no, const char* err_msg) {
    string json = "{\"result\":\"N\",\"msg\":\"";
    json += err_msg;
    json += "\"}";
    evbuffer_add_printf(evb, "%s", json.c_str());
    if (err_no < 0) {
        TRACE_ERROR(MODULE_NAME, MANAGER_E_SERVER_NOTIFY_DATA_INVALID,
                "error %d ->%s(%s)\n", err_no, addr, request);
        return;
    }
    TRACE_ERROR(MODULE_NAME, MANAGER_E_SERVER_NOTIFY_DATA_INVALID,
            "error %d:%s ->%s(%s)\n", err_no, err_msg, addr, request);

}

inline void fromat_success(struct evbuffer *evb, base::dictionary &dict) {
    base::dictionary result_dict;
    result_dict.add_object("result", "Y");
    result_dict.add_object("msg", dict);
    evbuffer_add_printf(evb, "%s", base::djson::dict2str(result_dict).c_str());
}

#define response_error(msg) \
    send_error_response(evb, addr, mncg_query_part, nResult, msg);

#define check_fail_handler(msg) \
    string json = "{\"result\":\"N\",\"msg\":\"";\
        json += msg;\
        json += "\"}";\
    evbuffer_add_printf(evb, "%s", json.c_str());\

int http_account_create(struct evkeyvalq *output_Headers, struct evbuffer *evb,
        const char* addr, const char * mncg_query_part,
        struct evhttp_request *req, struct evkeyvalq &mncg_http_query);

void send_document_cb(struct evhttp_request* req, void* arg)
{
    struct evkeyvalq *output_Headers = evhttp_request_get_output_headers(req);
    evhttp_add_header(output_Headers, "Content-Type", "text/html; charset=utf-8");
    evhttp_add_header(output_Headers, "Connection", "keep-alive");
    evhttp_add_header(output_Headers, "Cache-Control", "no-cache");

    struct evhttp_connection *conn = evhttp_request_get_connection(req);
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

    if (mncg_input_opt != NULL && strlen(mncg_input_opt) >= 5) {
        int nResult = 0;
        if (strcmp(mncg_input_opt, "create_account") == 0) {
            nResult = http_account_create(output_Headers, evb, addr, mncg_query_part, req, mncg_http_query);
            if (nResult == -1) {
                goto deal_error;
            }
        }
        else {
            check_fail_handler("error uri");
            goto deal_error;
        }
    }
    else {
        check_fail_handler("error opt");
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


int http_account_create(struct evkeyvalq *output_Headers, struct evbuffer *evb,
        const char* addr, const char * mncg_query_part,
        struct evhttp_request *req, struct evkeyvalq &mncg_http_query)
{
    int nResult = 0;
    const char *user_id = evhttp_find_header(&mncg_http_query, "user_id");
    const char *fund = evhttp_find_header( &mncg_http_query, "fund");
    const char *broker = evhttp_find_header(&mncg_http_query, "broker");

    if(user_id == 0 || fund==0 || broker ==0 || strcmp(user_id, "") == 0 || strcmp(fund, "") == 0 || strcmp(broker, "") == 0 ){
        response_error("error param");
        return -1;
    }
    std::shared_ptr<base::dictionary> smart_pdict(new base::dictionary());
    (*smart_pdict)["user_id"] = user_id;
    (*smart_pdict)["broker"] = broker;
    (*smart_pdict)["fund"] = fund;

    base::dictionary tmp;
    //nResult = trade_req_deal::account_create(smart_pdict, tmp);
    if (nResult == 0) {
      fromat_success(evb, tmp);
    } else {
      response_error("error create");
      return -1;
    }

    return 0;
}



}
