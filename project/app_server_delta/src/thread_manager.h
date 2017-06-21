#ifndef __THREAD_MANAGER_H__
#define __THREAD_MANAGER_H__

////////////////////////////////////////////////////////////////////////////////

#include "nocopyable.h"
#include <list>
#include <thread>
#include <iostream>
#include <functional>
#include <memory>

namespace serverframe{;


////////////////////////////////////////////////////////////////////////////////
class thread_manager : public nocopyable
{
public:
    /*@ start thread.
    * @ para.func: std::function object, it can use the "std::bind" method.
    */
    void run_thread(std::function<void(void)> func, size_t service_size = 1)
    {
        for (size_t srv = 0; srv<service_size; ++srv) {
            m_threadgroup.push_back(std::make_shared<std::thread>(func));
        }
    }

    void interrupt_all()
    {
    }

    void join_all()
    {
        for (auto it = m_threadgroup.begin(); it != m_threadgroup.end(); it++) {
            if (*it != nullptr) {
                (*it).get()->join();
            }
        }
        m_threadgroup.clear();
    }

private:
    // ����������߳���, �����д洢��ָ���̵߳Ĺ���ָ��
    std::list<std::shared_ptr<std::thread>> m_threadgroup;
};



}// serverframe


////////////////////////////////////////////////////////////////////////////////
#endif
