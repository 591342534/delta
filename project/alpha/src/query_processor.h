/*****************************************************************************
 returns_report_manager Copyright (c) 2016. All Rights Reserved.

 FileName: query_processor.h
 Version: 1.0
 Date: 2016.03.15

 History:
 cwm     2016.03.15   1.0     Create
 david wang     2016.03.16   1.0     modfiy
 ******************************************************************************/

#ifndef __QUERY_PROCESSOR_H__
#define __QUERY_PROCESSOR_H__

#include "common.h"
#include "processor_base.h"
#include "product_business_deal.h"
#include "base/dictionary.h"
#include "base/event.h"
#include "database/unidbpool.h"
#include <string>

namespace serverframe
{

class query_processor : public processor_base
{
public:
    query_processor();
    ~query_processor();

public:
    int start(const char* server_name);
    virtual void stop();

    virtual void post(const atp_message& msg);

protected:
    virtual void run();

private:
    int start_internal();
    int stop_internal();

    int rsp_entrust_result_deal(base::dictionary &dict);
    int rsp_deal_result_deal(base::dictionary &dict);
    int redo_exception_order(base::dictionary &dict);

private:
    std::string server_name_;
    base::event* msg_event_;
    bool started_;
};

}

#endif /* TRUNK_MODULES_SIGMA_STOCK_T0_SOURCE_RETURNS_REPORT_MANAGER_SRC_QUERY_PROCESSOR_H_ */
