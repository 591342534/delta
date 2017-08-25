#ifndef AFW_LOG_SEVERITY_H
#define AFW_LOG_SEVERITY_H

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

//@ returns stringized enumeration value or \c NULL, if the value is not valid.
const char* ToString(SeverityLevel lvl);

//@ outputs stringized representation of the severity level to the stream
template< typename CharT, typename TraitsT >
inline std::basic_ostream< CharT, TraitsT >& operator<< (
	std::basic_ostream< CharT, TraitsT >& strm,
	SeverityLevel lvl)
{
	const char* str = ToString(lvl);
	if (str)
		strm << str;
	else
		strm << static_cast< int >(lvl);
	return strm;
}

#endif