/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: test_log.h
Version: 1.0
Date: 2016.1.13

History:
ericsheng     2016.1.13   1.0     Create
******************************************************************************/

#ifndef __TEST_LOG_H__
#define __TEST_LOG_H__

#include "test_common.h"
#include <boost/log/trivial.hpp>
#include "log/Log.h"
class test_log
{
public:
    test_log(){}
    ~test_log(){}

    void test()
    {
        get_Logger().Init("", "");
        //get_Logger().InitLoggingSink(false, true);
        get_Logger().InitPersistSink(false, true);
        // 若设置为False（异步控制台输出）会导致界面阻塞。
        //get_Logger().InitConsoleSink();
        get_Logger().Filter(info);

        for (int i = 0; i < 100; i++)
        {
            AfwInfo << "boost::thread::hardware_concurrency() = " << i;
        }
        cout << ">>>>>>>>>>>>>>>>>>>>hello>>>>>>>>>>>>>>>>>>>" << endl;
    }
};

#endif
