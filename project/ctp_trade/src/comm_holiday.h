/*****************************************************************************
order_manager Module Copyright (c) 2015. All Rights Reserved.

FileName: common_holiday.h
Version: 1.0
Date: 2017.08.08

History:
eric     2017.08.08   1.0     Create
******************************************************************************/

#ifndef __COMMON_HOLIDAY_H__
#define __COMMON_HOLIDAY_H__

#include "base/util.h"
#include "base/dictionary.h"
#include "base/trace.h"
#include "database/unidbpool.h"
#include "common.h"
#include <string.h>
#include <string>
#include <time.h>

using namespace std;

namespace ctp
{
class comm_holiday
{
public:
    comm_holiday();
    ~comm_holiday();

public:
    bool load_holiday(database::db_instance* dbconn, const std::string &table);
    bool get_latestholiday(time_t* latest_holiday);
    time_t get_next_work_day(time_t tdate);
    time_t get_last_work_day(time_t tdate);
    time_t add_work_days(time_t tdate,int ndays);
    bool isholiday(time_t tdate);
   
    std::string get_trade_day();
    int get_today_second();

private:
    std::vector<std::string> holidaysvec_;  //节假日列表
};

}
#endif
