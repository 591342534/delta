#ifndef LOGGER_H_
#define LOGGER_H_

#include <string>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/noncopyable.hpp>
#include <iostream>

using namespace std;

//@ afw severity level.
enum SeverityLevel
{
    trace = 0,
    debug,
    info,
    warn,
    error,
    fatal,

    test
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
        "trace",
        "debug",
        "info ",
        "warn ",
        "error",
        "fatal",

        // test use.
        "test ",
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

////////////////////////////////////////////////////////////////////////////////
class Logger
{
private:
    Logger();
////////////////////////////////////////////////////////////////////////////////
public:
	typedef boost::log::sources::severity_logger_mt<
		SeverityLevel> logger_mt;

    ~Logger();

    static Logger& Instance()
    {
        static Logger instance_;
        return instance_;
    }

    void Init(
        std::string module_name = "",
        std::string process_id = "");

	// init console sink.
	void InitConsoleSink();

	// init file sink.
	void InitLoggingSink(
		bool is_sync,
		bool is_auto_flush);

	// persist logging.
	void InitPersistSink(
		bool is_sync,
		bool is_auto_flush);

	// get logger object.
	logger_mt& GetMt();

	// set module name.
	void SetModuleName(
		const std::string& module_name);

	// set process id.
	void SetProcessId(
		const std::string& process_id);

	// flush logs to backend output.
	void Flush();

	/*@ open logging.*/
	void Enable(bool is_enabled = true);

	/*@ filter log by serverity.*/
	void Filter(SeverityLevel sev = debug);

private:
	class impl;
	impl* m_impl;
};

////////////////////////////////////////////////////////////////////////////////
#endif