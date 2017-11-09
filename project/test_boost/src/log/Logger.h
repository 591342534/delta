#ifndef LOGGER_H_
#define LOGGER_H_

#include <string>
#include <iostream>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/noncopyable.hpp>

//@ afw severity level.
enum SeverityLevel
{
    trace = 0,
    debug,
    info,
    warning,
    error,
    critical
};

//@ outputs stringized representation of the severity level to the stream
template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (
    std::basic_ostream< CharT, TraitsT >& strm,
    SeverityLevel lvl)
{
    static const char* const str[] =
    {
        // log use.
        "trace___",
        "debug___",
        "info____",
        "warning_",
        "error___",
        "critical"
    };
    //const char* str = ToString(lvl);
    if (static_cast<size_t>(lvl) < (sizeof(str) / sizeof(*str))) {
        strm << str[lvl];
    }
    else {
        strm << static_cast<int>(lvl);
    }

    return strm;
}
namespace light
{

    class Logger
    {
    private:
        Logger();

    public:
        typedef boost::log::sources::severity_logger_mt<
            SeverityLevel> logger_mt;

        ~Logger();

        static Logger& Instance()
        {
            static Logger instance_;
            return instance_;
        }

        // 异步分等级输出日志
        void InitLevelLog(const std::string & filename);

        // init console sink.
        void InitConsoleLog(SeverityLevel sev = error);

        // init persist logging.
        void InitPersistLog(
            bool is_sync,
            bool is_auto_flush);

        /*@ filter log by serverity.*/
        void Filter(SeverityLevel sev = debug);

        // flush logs to backend output.
        void Flush();

        /*@ open logging.*/
        void Enable(bool is_enabled = true);

        // get logger object.
        logger_mt& GetMt();

    private:
        class impl;
        impl* m_impl;
    };
}
#endif