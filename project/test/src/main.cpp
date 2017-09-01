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
    void operator()(int i)
    {
        cout << "operator()" << i << endl;
    }
};

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
    vector<int> tt = { 1, 2, 3, 4, 5 };
    auto it = tt.begin();
    it = it + 3;
    cout << *it << endl;
    getchar();
    return 0;
}