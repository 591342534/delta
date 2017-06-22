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

using namespace std;

class test {
public:
    test(int i = default_num) 
        : num(i){}

private:
    static int default_num;
public:
    int num;
};

int test::default_num = 100;

int main(int argc, char *argv[])
{
    test one;
    cout << one.num << endl;
    test two(200);
    cout << two.num << endl;

    getchar();
    return 0;
}