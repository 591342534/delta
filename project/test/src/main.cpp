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
#include <algorithm>
#include <utility>
#include <tuple>
#include <chrono>
#include <time.h>
using namespace std;

class Graphics
{
public:
    void test();
private:
    class Impl;
    std::shared_ptr<Impl> impl;
};

class Graphics::Impl
{
public:
    void test();
};

void Graphics::Impl::test()
{
    cout << __FUNCTION__ << endl;
}

void Graphics::test()
{
    impl->test();
}

template <typename T>
void foo_impl(T val, true_type)
{
    cout << "one" << endl;
}

template <typename T>
void foo_impl(T val, false_type)
{
    cout << "two" << endl;
}

template <typename T>
void foo(T val)
{
    foo_impl(val, std::is_integral<T>());
}


int main(int argc, char *argv[])
{
    chrono::seconds one(20);
    chrono::hours two(10);
    std::time_t t = chrono::steady_clock::to_time_t(chrono::steady_clock::now());
    cout << time(NULL) << endl;
    cout << clock() / CLOCKS_PER_SEC << endl;
    getchar();
    return 0;
}