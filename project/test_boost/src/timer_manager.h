#include <set>
#include <deque>
#include <algorithm>
#include <functional>
#include <memory>

#include <boost/thread/mutex.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>

class TimerManager
{
public:
    typedef std::shared_ptr<boost::asio::io_service::work> work_ptr;
    typedef std::shared_ptr<boost::thread> thread_ptr;
    typedef std::function<void(void)> timer_callback;

public:
    static TimerManager& get_timer_manager()
    {
        static TimerManager inst;
        return inst;
    }
    boost::asio::io_service& get_io_service() { return processor_service_; };
    void RegisterTimer(int timeinterval, timer_callback cb);
    void start();
private:
    void run(boost::asio::io_service* io_service);

private:
    class Timer
        : public std::enable_shared_from_this<Timer>
    {
    public:
        Timer(int interval);
        void Register(timer_callback cb);
        void start();
        void handler_timer(const boost::system::error_code& error);
    private:
        std::shared_ptr<boost::asio::deadline_timer> timer_;
        std::vector< timer_callback > processors_;
        int interval_;
    };

private:
    boost::asio::io_service processor_service_;
    thread_ptr iothread_;
    work_ptr work_;
    std::map<int, std::shared_ptr<Timer> > clocks_;
};

