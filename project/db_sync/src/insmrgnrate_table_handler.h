/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: insmrgnrate_table_handler.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/

#ifndef __INSMRGNRATE_TABLE_HANDLER_H__
#define __INSMRGNRATE_TABLE_HANDLER_H__

#include "server.h"

namespace db_sync{;

class insmrgnrate_table_handler : public table_handler
{
public:
    virtual ~insmrgnrate_table_handler(){}
    virtual int oracle_to_mysql(void);
};

}
#endif
