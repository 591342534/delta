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
    explicit test(int i) 
        : num(i){}

public:
    int num;
};

int main(int argc, char *argv[])
{
    char *s = "abcde";
    s += 2;
    printf("%d", s);



    getchar();
    return 0;
}