#include <iostream>
#include <string>
#include <sstream>
#include <map>
#include <signal.h>
#include <stdio.h>
#include <memory>
#include <vector>
#include <set>
#include <assert.h>
#include <queue>
#include <unordered_map>
#include <functional>
#include <numeric>
#include <stack>
#include <deque>
#include <queue>
#include <thread>
#include <chrono>
#include <mutex>
#include "utility/applog.h"
#include <regex>

using namespace std;

class test_operator
{
public:
	void operator()(int &x)
	{
		x *= x;
	}
};

template <typename TP> 
struct COne 
{   // default member is public
	typedef TP one_value_type;
}; 
template <typename COne>   // 用一个模板类作为模板参数, 这是很常见的
struct CTwo 
{
	// 请注意以下两行
	//typedef COne::one_value_type  two_value_type;   // *1  原来这里为Cone:one_value我改成Cone::value
	typedef typename COne::one_value_type  two_value_type; // *2  原来这里为Cone:one_value我改成Cone::value
}; 


class base
{
    SINGLETON_ONCE(base);

public:
	virtual void print()
	{
		while (1) {
            static int num;
			cout << "this is " << num++ << endl;
			
			Sleep(1000);
		}
	}
};
SINGLETON_GET(base);

int main(int argc, char *argv[])
{
    map<int, string> m;
    m[1] = "one";
    m[2] = "two";
    m[3] = "three";
    m[4] = "four";
    m[5] = "five";
    m[6] = "six";
    for (auto rit = m.rbegin(); rit != m.rend();) {
        cout << rit->first << endl;
        m.erase(rit->first);
    }

    for (auto it = m.begin(); it != m.end(); it++) {
        cout << it->first << ":" << it->second << endl;
    }
	getchar();
	return 0;
}


