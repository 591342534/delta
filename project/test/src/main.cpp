#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <string.h>

#include <iostream>
#include <string>
#include <sstream>
#include <memory>
#include <vector>
#include <set>
#include <queue>
#include <stack>
#include <deque>
#include <map>
#include <unordered_map>
#include <functional>
#include <numeric>
#include <thread>
#include <chrono>
#include <mutex>

#include <iomanip>
#include <log4cplus/logger.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/fileappender.h>
using namespace log4cplus;
using namespace log4cplus::helpers;

using namespace std;

class test {
public:
    explicit test(int i) 
        : num(i){}

public:
    int num;
};


int main(int argc, char *argv[])
{

    getchar();
    return 0;
}