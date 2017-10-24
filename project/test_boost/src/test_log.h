/******************************************************************************
Copyright (c) 2016. All Rights Reserved.

FileName: test_log.h
Version: 1.0
Date: 2016.1.13

History:
eric     2016.1.13   1.0     Create
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
        Logger::Instance().Init("", "");
        // 文件日志
        // 服务运行测试阶段：使用实时同步的日志策略，以便于跟踪问题。(当前)
        // 服务正常运行阶段：使用非实时异步的日志策略，以提高性能。
        Logger::Instance().InitPersistSink(true, true);
        Logger::Instance().InitConsoleSink();
        Logger::Instance().Filter(info);

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
