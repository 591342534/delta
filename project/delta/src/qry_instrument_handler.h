﻿/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: qry_instrument_handler.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#ifndef __QRY_ORDER_HANDLER_H__
#define __QRY_ORDER_HANDLER_H__
////////////////////////////////////////////////////////////////////////////////
#include "server.h"

namespace serverframe{;
////////////////////////////////////////////////////////////////////////////////
class qry_instrument_handler : public request_handler
{
////////////////////////////////////////////////////////////////////////////////
public:
public:
    void on_request(Context& context);
};


////////////////////////////////////////////////////////////////////////////////
}// serverframe
#endif
