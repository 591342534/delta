#ifndef AFW_LOG_LOGGER_IPP
#define AFW_LOG_LOGGER_IPP
////////////////////////////////////////////////////////////////////////////////
#include "Logger.h"
#include <boost/log/sinks.hpp>
#include <boost/log/common.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


////////////////////////////////////////////////////////////////////////////////
namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;
namespace attrs = boost::log::attributes;
namespace sinks = boost::log::sinks;

////////////////////////////////////////////////////////////////////////////////
/*@ const attribute names.*/
namespace log_attribute_names{;

inline const char* line_id()
{
	return "lid";
}

inline const char* timestamp()
{
	return "tsp";
}

inline const char* process_id()
{
	return "pid";
}

inline const char* thread_id()
{
	return "tid";
}

inline const char* severity()
{
	return "sev";
}

inline const char* module()
{
	return "mod";
}

inline const char* scope()
{
	return "scp";
}

inline const char* message()
{
	return "msg";
}

// inner name, cann't change.
inline const char* inner_severity()
{
	return "Severity";
}
}// ns::log_attr


////////////////////////////////////////////////////////////////////////////////
/*@ extract attribute placeholder.*/
BOOST_LOG_ATTRIBUTE_KEYWORD(attr_line_id, 
	log_attribute_names::line_id(), unsigned int);
BOOST_LOG_ATTRIBUTE_KEYWORD(attr_timestamp, 
	log_attribute_names::timestamp(), 
	attrs::local_clock::value_type);
BOOST_LOG_ATTRIBUTE_KEYWORD(attr_thread_id, 
	log_attribute_names::thread_id(),
	attrs::current_thread_id::value_type);
BOOST_LOG_ATTRIBUTE_KEYWORD(attr_process_id, 
	log_attribute_names::process_id(),
	std::string);
BOOST_LOG_ATTRIBUTE_KEYWORD(attr_severity, 
	log_attribute_names::inner_severity(),
	SeverityLevel);
BOOST_LOG_ATTRIBUTE_KEYWORD(attr_module, 
	log_attribute_names::module(), std::string);
BOOST_LOG_ATTRIBUTE_KEYWORD(attr_scope, 
	log_attribute_names::scope(),
	attrs::named_scope::value_type);

////////////////////////////////////////////////////////////////////////////////
class Logger::impl
{
public:
	// console sink.
	typedef sinks::asynchronous_sink<sinks::text_ostream_backend> 
		async_console_sink;
	typedef sinks::synchronous_sink<sinks::text_ostream_backend> 
		sync_console_sink;
	// file sink.
	typedef sinks::asynchronous_sink<sinks::text_file_backend> 
		async_file_sink;
	typedef sinks::synchronous_sink<sinks::text_file_backend> 
		sync_file_sink;

public:
	impl();

	// init attributes.
	inline void InitAttributes();

	// init console sink.
	template<typename sink_type>
	inline void InitConsoleSink();

	// init file sink.
	template<typename sink_type>
	inline void InitLoggingSink(bool is_auto_flush);

	template<typename sink_type>
	inline void InitPersistSink(bool is_auto_flush);

	// get logger object.
	inline Logger::logger_mt& GetLogger()
	{
		return m_logger;
	}

	// set module name.
	inline void SetModuleName(
		const std::string& module_name);

	// set process id.
	inline void SetProcessId(
		const std::string& id);

	// flush logs.
	inline void Flush();

private:
	Logger::logger_mt m_logger;
	attrs::mutable_constant<std::string> m_module_attr;
	attrs::mutable_constant<std::string> m_process_id;
};


////////////////////////////////////////////////////////////////////////////////
Logger::impl::impl() : m_module_attr(""), m_process_id("")
{
}


////////////////////////////////////////////////////////////////////////////////
void Logger::impl::InitAttributes()
{
	// add "LineID"\"TimeStamp"\"ProcessID"\"ThreadID";
	// logging::add_common_attributes();
	logging::core::get()->add_global_attribute(
		log_attribute_names::line_id(),
		attrs::counter<unsigned int>(1));
	logging::core::get()->add_global_attribute(
		log_attribute_names::timestamp(),
		attrs::local_clock());
	logging::core::get()->add_global_attribute(
		log_attribute_names::process_id(),
		m_process_id);
	logging::core::get()->add_global_attribute(
		log_attribute_names::thread_id(),
		attrs::current_thread_id());

	// add "Module"\"Scope"
	m_module_attr.set("");
	logging::core::get()->add_global_attribute(
		log_attribute_names::module(),
		m_module_attr);
	logging::core::get()->add_global_attribute(
		log_attribute_names::scope(),
		attrs::named_scope());
}


////////////////////////////////////////////////////////////////////////////////
void Logger::impl::SetModuleName(const std::string& module_name)
{
	m_module_attr.set(module_name);
}
void Logger::impl::SetProcessId(const std::string& id)
{
	m_process_id.set(id);
}

////////////////////////////////////////////////////////////////////////////////
void Logger::impl::Flush()
{
	logging::core::get()->flush();
}


////////////////////////////////////////////////////////////////////////////////
template<typename sink_type>
void Logger::impl::InitConsoleSink()
{
	// synchronous sink to display on console.
	boost::shared_ptr<sink_type> console_sink;
	console_sink = boost::make_shared<sink_type>();

	console_sink->locked_backend()->add_stream(
		boost::shared_ptr<std::ostream>(
		&std::clog, boost::null_deleter()));

	console_sink->set_formatter(expr::format("> [%1%] [%2%] [%3%] : %4%")
		%expr::format_date_time(attr_timestamp,"%Y-%m-%d %H:%M:%S.%f")
		%attr_thread_id %attr_severity %expr::smessage);
	logging::core::get()->add_sink(console_sink);
}


////////////////////////////////////////////////////////////////////////////////
template<typename sink_type>
void Logger::impl::InitLoggingSink(bool is_auto_flush)
{
	/*@ asynchronous sink to file.*/
	// construct backend.
	boost::shared_ptr<sinks::text_file_backend> backend =
		boost::make_shared<sinks::text_file_backend>(
		keywords::open_mode = std::ios::app,
		keywords::file_name = "./log/%Y%m%d.log",
		keywords::rotation_size = 500*1024*1024,			// 500M
		keywords::time_based_rotation = 
		sinks::file::rotation_at_time_point(0, 0, 0)
		);
	backend->auto_flush(is_auto_flush);

	// construct sink.
	boost::shared_ptr<sink_type> file_sink(new sink_type(backend));
	file_sink->set_formatter(expr::format("[%1%] [%2%] [%3%] : %4%")
		%expr::format_date_time(attr_timestamp,"%Y-%m-%d %H:%M:%S.%f")
		%attr_thread_id %attr_severity %expr::smessage);
	logging::core::get()->add_sink(file_sink);
}


////////////////////////////////////////////////////////////////////////////////
template<typename sink_type>
void Logger::impl::InitPersistSink(bool is_auto_flush)
{
	/*@ asynchronous sink to file.*/
	// construct backend.
	boost::shared_ptr<sinks::text_file_backend> backend =
		boost::make_shared<sinks::text_file_backend>(
		keywords::open_mode = std::ios::app,
		keywords::file_name = "./log/%Y%m%d.log",
		keywords::rotation_size = 500*1024*1024,			// 500M
		keywords::time_based_rotation = 
		sinks::file::rotation_at_time_point(0, 0, 0));
	backend->auto_flush(is_auto_flush);

	// construct sink.
	boost::shared_ptr<sink_type> file_sink(new sink_type(backend));
	file_sink->set_formatter(expr::format(
		"%1%=%2% %3%=%4% %5%=%6% %7%=%8% %9%=%10% %11%=%12% %13%=%14%")
		%log_attribute_names::line_id() % attr_line_id
		%log_attribute_names::timestamp() 
		%expr::format_date_time(attr_timestamp,"%Y-%m-%d %H:%M:%S.%f")
		%log_attribute_names::process_id()% attr_process_id
		%log_attribute_names::thread_id() % attr_thread_id
		%log_attribute_names::scope()% attr_scope
		%log_attribute_names::severity()% attr_severity
		%log_attribute_names::message() % expr::smessage);
	logging::core::get()->add_sink(file_sink);
}



////////////////////////////////////////////////////////////////////////////////
Logger::Logger()
{
    m_impl = new impl();
}


////////////////////////////////////////////////////////////////////////////////
Logger::~Logger()
{
    delete m_impl;
    m_impl = nullptr;
}


////////////////////////////////////////////////////////////////////////////////
void Logger::Init(
	std::string module_name,
	std::string proc_id)
{
    m_impl->InitAttributes();
    m_impl->SetModuleName(module_name);
    m_impl->SetProcessId(proc_id);
    Filter(debug);
}



////////////////////////////////////////////////////////////////////////////////
void Logger::InitConsoleSink()
{
	m_impl->InitConsoleSink<impl::sync_console_sink>();
}


////////////////////////////////////////////////////////////////////////////////
void Logger::InitLoggingSink(bool is_sync, bool is_auto_flush)
{
	if (is_sync)
	{
		m_impl->InitLoggingSink<impl::sync_file_sink>(is_auto_flush);
	}
	else
	{
		m_impl->InitLoggingSink<impl::async_file_sink>(is_auto_flush);
	}
}


////////////////////////////////////////////////////////////////////////////////
void Logger::InitPersistSink(bool is_sync, bool is_auto_flush)
{
	if (is_sync)
	{
		m_impl->InitPersistSink<impl::sync_file_sink>(is_auto_flush);
	}
	else
	{
		m_impl->InitPersistSink<impl::async_file_sink>(is_auto_flush);
	}
}


////////////////////////////////////////////////////////////////////////////////
Logger::logger_mt& Logger::GetMt()
{
	return m_impl->GetLogger();
}
void Logger::SetModuleName(const std::string& module_name)
{
	m_impl->SetModuleName(module_name);
}
void Logger::SetProcessId(const std::string& id)
{
	m_impl->SetProcessId(id);
}
void Logger::Flush()
{
	m_impl->Flush();
}
void Logger::Enable(bool is_enabled)
{
	logging::core::get()->set_logging_enabled(is_enabled);
}
void Logger::Filter(SeverityLevel sev)
{
	logging::core::get()->set_filter(attr_severity >= sev);
}


////////////////////////////////////////////////////////////////////////////////
#endif