/*****************************************************************************
 returns_report_manager Copyright (c) 2016. All Rights Reserved.

 FileName: http_unit.h
 Version: 1.0
 Date: 2016.03.29

 History:
 david wang     2016.03.29   1.0     create
 ******************************************************************************/

#ifndef TRUNK_MODULES_SIGMA_STOCK_T0_SOURCE_RETURNS_REPORT_MANAGER_HTTP_UNIT_H_
#define TRUNK_MODULES_SIGMA_STOCK_T0_SOURCE_RETURNS_REPORT_MANAGER_HTTP_UNIT_H_

#include "common.h"
#include "processor_base.h"
#include "base/event.h"
#include "base/http_client.h"

namespace serverframe
{

class http_unit : public processor_base
{
public:
    http_unit();
    virtual ~http_unit();

public:
    int start(const char* server_name);
    virtual void stop();
    virtual void post(const atp_message& msg);

protected:
    int start_internal();
    int stop_internal();

protected:
    virtual void run();

    static void post_msg_cb(char* des, size_t size);
    static void get_msg_cb(char* des, size_t size);
private:
    std::string server_name_;
    base::event* msg_event_;
    bool started_;

    base::http_client m_http_client_;
};

}

#endif /* TRUNK_MODULES_SIGMA_STOCK_T0_SOURCE_RETURNS_REPORT_MANAGER_HTTP_UNIT_H_ */
