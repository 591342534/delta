/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: qry_expireinst_handler.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.5.25   1.0     Create
******************************************************************************/

#ifndef __QRY_CALENDAR_HANDLER_H__
#define __QRY_CALENDAR_HANDLER_H__
////////////////////////////////////////////////////////////////////////////////
#include "server.h"

namespace serverframe{;
////////////////////////////////////////////////////////////////////////////////
class qry_expireinst_handler : public request_handler
{
////////////////////////////////////////////////////////////////////////////////
public:
public:
    void on_request(Context& context);
};


////////////////////////////////////////////////////////////////////////////////
}// serverframe
#endif
