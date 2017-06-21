#ifndef __TEST_SINGLETON_H__
#define __TEST_SINGLETON_H__

#include <stdio.h>
#include <iostream>

#include "utility/singleton.h"

using namespace std;

class test_singleton
	: public utility::singleton<test_singleton> {
private:
	test_singleton() {}
	virtual ~test_singleton() {}

	friend class utility::singleton<test_singleton>;

public:
	void test()
	{
		cout << "this is singleton_test" << endl;
	}
};

#endif