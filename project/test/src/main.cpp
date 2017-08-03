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
class A;
class B
{
public:
    B(shared_ptr<A> tmp)
        : one(tmp)
    {

    }
private:
    shared_ptr<A> one;
};
class A : public enable_shared_from_this<A> {
public:
    A() {
        cout << "A::A()" << endl;
    }

    ~A() {
        cout << "A::~A()" << endl;
    }
    void test()
    {
        bb.reset(new B(shared_from_this()));
    }

private:
    int x_;
    shared_ptr<B> bb;
};



int main(int argc, char *argv[])
{
    std::vector<int> one = {3, 4, 99, 22, 10, 9};

    cout << *std::min_element(one.begin(), one.end()) << endl;

    getchar();
    return 0;
} 