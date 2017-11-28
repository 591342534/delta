#include <iostream>
#include "timer_manager.h"

void TimerManager::RegisterTimer(int timeinterval, timer_callback cb)
{
    auto it = clocks_.find(timeinterval);
    if (it != clocks_.end())
    {
        it->second->Register(cb);
    }
    else
    {
        std::shared_ptr<Timer> timer = std::make_shared<Timer>(timeinterval);
        timer->Register(cb);
        clocks_[timeinterval] = timer;
    }
}

void TimerManager::start()
{
    work_ = std::make_shared<boost::asio::io_service::work>(processor_service_);
    iothread_ = thread_ptr(new boost::thread(std::bind(&TimerManager::run, this, &processor_service_)));
    for (auto it = clocks_.begin(); it != clocks_.end(); ++it) {
        if (it->second) {
            it->second->start();
        }
    }
    //std::cout << "TimerManager processor thread started... threadid: " << iothread_->get_id();
}


void TimerManager::run(boost::asio::io_service* io_service)
{
    io_service->run();
    std::cout << " io service loop end";
}

TimerManager::Timer::Timer(int interval)
    : interval_(interval)
{
}

void TimerManager::Timer::Register(timer_callback cb)
{
    processors_.push_back(cb);
}

void TimerManager::Timer::start()
{
    timer_ = std::make_shared<boost::asio::deadline_timer>(
        TimerManager::get_timer_manager().get_io_service(), 
        boost::posix_time::seconds(interval_));
    timer_->expires_from_now(boost::posix_time::milliseconds(interval_));
    timer_->async_wait(std::bind(&TimerManager::Timer::handler_timer, shared_from_this(), std::placeholders::_1));
}

void TimerManager::Timer::handler_timer(const boost::system::error_code& error)
{
    if (error != boost::asio::error::operation_aborted) {
        // Timer was not cancelled, take necessary action.
        for (auto it = processors_.begin(); it != processors_.end(); ++it) {
            if (*it) {
                (*it)();
            }
        }
        timer_->expires_from_now(boost::posix_time::milliseconds(interval_));
        timer_->async_wait(std::bind(&TimerManager::Timer::handler_timer, shared_from_this(), std::placeholders::_1));
    }
}
