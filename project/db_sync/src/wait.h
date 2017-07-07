/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: wait.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.4.13   1.0     Create
******************************************************************************/
#ifndef __WAIT_H__
#define __WAIT_H__

#include <functional>

#include "base/util.h"

namespace db_sync{;
////////////////////////////////////////////////////////////////////////////////
inline void sleep_cross(const int mill_secs)
{
    if (mill_secs > 0) {
        base::util::sleep(mill_secs);
    }
}


////////////////////////////////////////////////////////////////////////////////
typedef std::function<bool (void)> checker_func;
////////////////////////////////////////////////////////////////////////////////
inline void normal_wait(checker_func checker, int unit_mill_sec = 50)
{
    if (unit_mill_sec < 10) {
        unit_mill_sec = 10;
    }

    while (!checker()) {
        sleep_cross(unit_mill_sec);
    }
}


////////////////////////////////////////////////////////////////////////////////
inline bool timed_wait(
    checker_func checker,
    const int timeout_mill_secs,
    int unit_mill_sec = 100)
{
    if (unit_mill_sec < 10) {
        unit_mill_sec = 10;
    }

    int count_mill_secs = 0;
    while (!checker() && count_mill_secs < timeout_mill_secs) {
        sleep_cross(unit_mill_sec);
        count_mill_secs += unit_mill_sec;
    }
    return count_mill_secs < timeout_mill_secs;
}

////////////////////////////////////////////////////////////////////////////////
}//serverframe
#endif
