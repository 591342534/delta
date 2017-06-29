/*****************************************************************************
 returns_report_manager Copyright (c) 2016. All Rights Reserved.

 FileName: deals_processor.h
 Version: 1.0
 Date: 2016.03.15

 History:
 cwm     2016.03.15   1.0     Create
 david wang     2016.03.16   1.0     modfiy
 ******************************************************************************/

#ifndef __DEALS_PROCESSOR_H__
#define __DEALS_PROCESSOR_H__

#include "common.h"
#include "processor_base.h"
#include "product_business_deal.h"
#include "database/unidbpool.h"
#include "base/dictionary.h"
#include "base/event.h"
#include <string>

using std::string;

namespace serverframe
{

struct ev_uri_component{
    string ip;
    string path;
    unsigned int ui_port ;
    ev_uri_component():ui_port(8080)
    {
    }
};

class deals_processor : public processor_base
{
public:
    deals_processor();
    ~deals_processor();

    int start(const char* server_name);
    virtual void stop();

    virtual void post(const atp_message& msg);
protected:
    virtual void run();

private:
    int start_internal();
    int stop_internal();

    int entrust_result_deal(base::dictionary &dict);
    int deal_result_deal(base::dictionary &dict);
    int withdraw_result_deal(base::dictionary &dict);

private:
    std::string server_name_;
    base::event* msg_event_;
    bool started_;
};

}

#endif 
