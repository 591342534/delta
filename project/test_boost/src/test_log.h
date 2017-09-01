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

#include <chrono>
#include <thread>
#include "test_common.h"
#include "log/Log.h"
class test_log
{
public:
    test_log(){}
    ~test_log(){}

    void test()
    {
        get_Logger().Init("", "");
        // 若设置为False（异步控制台输出）会导致界面阻塞。
        get_Logger().InitLoggingSink(true, true);
        //get_Logger().InitConsoleSink();
        get_Logger().Filter(info);

        auto begin_clock = chrono::system_clock::now();
        for (int i = 0; i < 500; i++) {
            BOOST_ERROR << "boost::thread::hardware_concurrency() = " << i;
            BOOST_WARN << "boost::thread::hardware_concurrency() = " << i;
        }
        cout << chrono::duration_cast<chrono::milliseconds>
            (chrono::system_clock::now() - begin_clock).count() << endl;
    }
};

#endif
