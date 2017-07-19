/// \file		ComHeader.h
/// \brief		预编译头文件
/// \author		
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

#ifdef _WIN32
#pragma once
#endif

#ifdef _MSC_VER

#ifndef _WIN32_WINNT            // Specifies that the minimum required platform is BASEows Vista.
#define _WIN32_WINNT 0x0501     // Change this to the appropriate value to target other versions of BASEows.
#endif	// _WIN32_WINNT

#endif // _MSC_VER

#include <string>
#include <list>

using namespace std;

#pragma warning(disable: 4267 4312 4311)

#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#pragma warning(default: 4267 4312 4311)

using namespace boost::asio;
using namespace boost::system;
using namespace boost::posix_time;

#ifndef _WIN32
#define __int64 long long
#endif

#include "ConstDef.h"
