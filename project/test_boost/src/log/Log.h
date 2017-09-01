#ifndef AFW_LOG_H
#define AFW_LOG_H

#include "Logger.h"
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/record_ostream.hpp>

//log use.
#define BOOST_TRACE BOOST_LOG_SEV(get_Logger().GetMt(), trace)
#define BOOST_DEBUG BOOST_LOG_SEV(get_Logger().GetMt(), debug)
#define BOOST_INFO  BOOST_LOG_SEV(get_Logger().GetMt(), info)
#define BOOST_WARN  BOOST_LOG_SEV(get_Logger().GetMt(), warn)
#define BOOST_ERROR BOOST_LOG_SEV(get_Logger().GetMt(), error)
#define BOOST_FATAL BOOST_LOG_SEV(get_Logger().GetMt(), fatal)
//test use.
#define BOOST_TEST  BOOST_LOG_SEV(get_Logger().GetMt(), test)

#endif