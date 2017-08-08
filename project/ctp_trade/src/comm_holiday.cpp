/*****************************************************************************
order_manager Module Copyright (c) 2015. All Rights Reserved.

FileName: common_holiday.cpp
Version: 1.0
Date: 2017.08.08

History:
eric     2017.08.08   1.0     Create
******************************************************************************/

#include "comm_holiday.h"

namespace ctp
{

comm_holiday::comm_holiday()
{

}

comm_holiday::~comm_holiday()
{

}

bool comm_holiday::load_holiday(database::db_instance* dbconn, const std::string &table)
{
    if (dbconn == NULL) {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_INIT_DATABASE_CONN_FAILED,
            "get db conn failed");
        return false;
    }
    char sql[512] = { 0 };
    sprintf(sql, "select * from %s", table.c_str());
    TRACE_SYSTEM(AT_TRACE_TAG, "load_holiday info,sql:%s", sql);

    if (dbconn->_conn->query(sql)) {
        holidaysvec_.clear();
        while (dbconn->_conn->fetch_row()) {
            std::string holiday = dbconn->_conn->get_string("holiday");
            if (holiday.empty()) {
                continue;
            }
            holidaysvec_.push_back(holiday);
        }
    }
    else {
        TRACE_ERROR(AT_TRACE_TAG, NAUT_AT_E_INIT_DATABASE_CONN_FAILED,
            "query sql fail:sql%s", sql);
        return false;
    }
    return true;
}

bool comm_holiday::get_latestholiday(time_t* latest_holiday)
{
    struct tm timeinfo = { 0 };
    time_t tmNow = time(NULL);
#ifdef _MSC_VER
    localtime_s(&timeinfo, &tmNow);
#else
    localtime_r(&tmNow, &timeinfo);
#endif

    if (timeinfo.tm_year == 0)
    {
        return false;
    }

    if (timeinfo.tm_wday == 0 || timeinfo.tm_wday == 6)
    {
        *latest_holiday = base::util::local_datestamp();
        return true;
    }

    long curdate = base::util::local_datestamp();
    for (int i = 0; i < (int)holidaysvec_.size(); i++)
    {
        if (base::util::string_to_datestamp((char *)holidaysvec_[i].c_str()) >= curdate)
        {
            *latest_holiday = base::util::string_to_datestamp((char *)holidaysvec_[i].c_str());
            return true;
        }
    }

    return false;
}

time_t comm_holiday::get_next_work_day(time_t tdate)
{
    time_t day = tdate + 24 * 3600;
    while (isholiday(day))
    {
        day += 24 * 3600;
    }

    return day;
}

time_t comm_holiday::get_last_work_day(time_t tdate)
{
    time_t day = tdate - 24 * 3600;
    while (isholiday(day))
    {
        day -= 24 * 3600;
    }

    return day;
}

time_t comm_holiday::add_work_days(time_t tdate, int ndays)
{
    time_t day = tdate;
    if (ndays <= 0)
    {
        return tdate;
    }

    do
    {
        day += 24 * 3600;
        if (isholiday(day))
        {
            continue;
        }

        ndays--;
    } while (ndays > 0);

    return day;
}

bool comm_holiday::isholiday(time_t tdate)
{
    struct tm * p_timeinfo;
    p_timeinfo = localtime(&tdate);
    if (p_timeinfo == NULL)
    {
        return false;
    }
    if (p_timeinfo->tm_wday == 0 || p_timeinfo->tm_wday == 6)
    {
        return true;
    }

    for (int i = 0; i < (int)holidaysvec_.size(); i++)
    {
        if (base::util::string_to_datestamp((char *)holidaysvec_[i].c_str()) == tdate)
        {
            return true;
        }
    }

    return false;
}

std::string comm_holiday::get_trade_day()
{
    string s = "";
    struct tm curTime = { 0 };
    time_t tmNow = time(NULL);

#ifdef _MSC_VER
    localtime_s(&curTime, &tmNow);
#else
    localtime_r(&tmNow, &curTime);
#endif
    /*如果当前时间小于6点，则交易时间取前一天的日期*/
    if (curTime.tm_hour < 6) {
        time_t yesterday = get_last_work_day(tmNow);
        memset(&curTime, 0, sizeof (curTime));

#ifdef _MSC_VER
        localtime_s(&curTime, &yesterday);
#else
        localtime_r(&yesterday, &curTime);
#endif
    }
    if (curTime.tm_year == 0)
    {
        return s;
    }

    char curday[11] = { 0 };
    sprintf(curday, "%04d-%02d-%02d", curTime.tm_year + 1900, curTime.tm_mon + 1, curTime.tm_mday);

    s = curday;

    return s;
}

int comm_holiday::get_today_second()
{
    struct tm curTime = { 0 };
    time_t tmNow = time(NULL);
#ifdef _MSC_VER
    localtime_s(&curTime, &tmNow);
#else
    localtime_r(&tmNow, &curTime);
#endif
    if (curTime.tm_year == 0)
        return 0;

    return curTime.tm_hour * 3600 + curTime.tm_min * 60 + curTime.tm_sec;
}

}