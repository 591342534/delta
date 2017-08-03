#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <map>
#include <list>
#include "boost/filesystem.hpp"
#include "boost/thread.hpp"
#include "boost/bind.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/asio.hpp>


class A;
class B
{
public:
    B(boost::shared_ptr<A> tmp)
        : one(tmp)
    {

    }
private:
    boost::shared_ptr<A> one;
};
class A : public boost::enable_shared_from_this<A> {
public:
    A() {
        std::cout << "A::A()" << std::endl;
    }

    ~A() {
        std::cout << "A::~A()" << std::endl;
    }
    void test()
    {
        bb.reset(new B(shared_from_this()));
    }

private:
    int x_;
    boost::shared_ptr<B> bb;
};

void test_func(boost::shared_ptr<A> tmp)
{
    boost::shared_ptr<A> aa = tmp;
    std::cout << aa.use_count() << std::endl;
}


int main()
{
    std::list<boost::shared_ptr<A> > list_aa;
    A *aa = new A();
    boost::shared_ptr<A> ss = boost::shared_ptr<A>(aa);
        
        
    std::cout << ss.use_count() << std::endl;
    list_aa.push_back(ss);
    std::cout << ss.use_count() << std::endl;

    getchar();
	return 0;
}
