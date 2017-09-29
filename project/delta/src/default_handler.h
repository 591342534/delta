/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: default_handler.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#ifndef __DEFAULT_HANDLER_H__
#define __DEFAULT_HANDLER_H__

#include "server.h"

namespace serverframe{;

class default_handler : public request_handler
{
public:
    void on_request(Context& context);
};

}// serverframe
#endif
