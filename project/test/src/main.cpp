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
#include <exception>
#include <iomanip>
#include <log4cplus/logger.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/fileappender.h>
using namespace log4cplus;
using namespace log4cplus::helpers;

using namespace std;

class Base
{
public:
    Base() 
    { 
        cout << "base()" << endl;
    }
    virtual ~Base() 
    {
        cout << "~base()" << endl;
    }
};
class Base1 :  public Base
{
public:
    Base1()
        : Base()
    {
        cout << "base1()" << endl;
    }
    virtual ~Base1()
    {
        cout << "~base1()" << endl;
    }

    void test(int b)
    {
        static int i = b;
        cout << b << endl;
    }
};

class my_exception : public exception
{
    const char* what() const
    {
        return "c++ exception";
    }
};
int main(int argc, char *argv[])
{
    
    try {
        throw 1;
    }
    //catch (my_exception &e){
    //    cout << "my_exception caught" << endl;
    //    cout << e.what() << endl;
    //}
    catch (std::exception& e) {
        cout << e.what() << endl;
    }
    //catch (...) {
    //    cout << "..." << endl;
    //}

    getchar();
    return 0;
} 