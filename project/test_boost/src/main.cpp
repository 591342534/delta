#include <iostream>
#include <sstream>
#include <string>
#include <map>

#include "boost/filesystem.hpp"
#include "boost/thread.hpp"
#include "boost/bind.hpp"

using namespace std;

class boost_thread
{
public:
    void test_func1(int count)
    {
        for (int i = 0; i < count; i++) {
            cout << "hello test_func1 " << i << endl;
        }
    }

    void test_func2()
    {
        while (1) {
            cout << "hello test_func2" << endl;
        }
    }
};

int main()
{
    boost_thread one;
    boost::thread thread_inst1(boost::bind(&boost_thread::test_func1, &one, 100));
    boost::thread thread_inst2(boost::bind(&boost_thread::test_func2, &one));
    thread_inst1.join();
    thread_inst2.detach();
    getchar();
	return 0;
}
