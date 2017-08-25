#ifndef AFW_LOG_H
#define AFW_LOG_H

#include "Logger.h"
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/record_ostream.hpp>

//log use.
#define AfwTrace BOOST_LOG_SEV(get_Logger().GetMt(), trace)
#define AfwDebug BOOST_LOG_SEV(get_Logger().GetMt(), debug)
#define AfwInfo  BOOST_LOG_SEV(get_Logger().GetMt(), info)
#define AfwWarn  BOOST_LOG_SEV(get_Logger().GetMt(), warn)
#define AfwError BOOST_LOG_SEV(get_Logger().GetMt(), error)
#define AfwFatal BOOST_LOG_SEV(get_Logger().GetMt(), fatal)
//test use.
#define AfwTest  BOOST_LOG_SEV(get_Logger().GetMt(), test)

inline const char* ReqTag()
{
	return "request  > ";
}

inline const char* RspTag()
{
	return "response > ";
}
#endif