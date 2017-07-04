/*****************************************************************************
 http_manager Copyright (c) 2016. All Rights Reserved.

 FileName: http_processor.h
 Version: 1.0
 Date: 2016.03.23

 History:
 cwm     2016.03.23   1.0     Create
 ******************************************************************************/

#ifndef TRUNK_MODULES_SIGMA_STOCKS_SOURCE_HTTP_MANAGER_SRC_HTTP_PROCESSOR_H_
#define TRUNK_MODULES_SIGMA_STOCKS_SOURCE_HTTP_MANAGER_SRC_HTTP_PROCESSOR_H_

#include "base/trace.h"
#include "base/logger.h"
#include "base/util.h"
#include "base/http_server.h"
#include "common.h"
namespace serverframe
{
class http_processor
{
public:
    http_processor();
    virtual ~http_processor();

public:
    int start();
    int stop();

public:
    static base::multi_thread_httpserver* http_;
};

}



#endif /* TRUNK_MODULES_SIGMA_STOCKS_SOURCE_HTTP_MANAGER_SRC_HTTP_PROCESSOR_H_ */
