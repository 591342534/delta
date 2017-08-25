#ifndef AFW_LOGGER_H
#define AFW_LOGGER_H

#include <string>
#include "singleton.h"
#include "Severity.h"
#include <boost/log/sources/severity_logger.hpp>
#include <boost/noncopyable.hpp>

////////////////////////////////////////////////////////////////////////////////
class Logger : boost::noncopyable
{
    SINGLETON_UNINIT(Logger);
    Logger();
////////////////////////////////////////////////////////////////////////////////
public:
	typedef boost::log::sources::severity_logger_mt<
		SeverityLevel> logger_mt;

    ~Logger();

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

SINGLETON_GET(Logger);
////////////////////////////////////////////////////////////////////////////////
#endif