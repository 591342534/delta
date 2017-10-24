#ifndef LOG_H_
#define LOG_H_

#include "Logger.h"
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/record_ostream.hpp>

//log use.
#define BOOST_TRACE BOOST_LOG_SEV(Logger::Instance().GetMt(), trace)
#define BOOST_DEBUG BOOST_LOG_SEV(Logger::Instance().GetMt(), debug)
#define BOOST_INFO  BOOST_LOG_SEV(Logger::Instance().GetMt(), info)
#define BOOST_WARN  BOOST_LOG_SEV(Logger::Instance().GetMt(), warn)
#define BOOST_ERROR BOOST_LOG_SEV(Logger::Instance().GetMt(), error)
#define BOOST_FATAL BOOST_LOG_SEV(Logger::Instance().GetMt(), fatal)
//test use.
#define BOOST_TEST  BOOST_LOG_SEV(Logger::Instance().GetMt(), test)

#endif