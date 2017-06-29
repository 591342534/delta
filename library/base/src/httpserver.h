/*****************************************************************************
 VBase Copyright (c) 2015. All Rights Reserved.

 FileName: httpserver.h
 Version: 1.0
 Date: 2015.10.11

 History:
 cwm     2015.10.11   1.0     Create
 ******************************************************************************/
 
#ifndef _HTTP_SERVER_H_
#define _HTTP_SERVER_H_

#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <evutil.h>
#include <event2/keyvalq_struct.h>
#include <event.h>
#include <evhttp.h>
#include <string>

#include <errno.h>
#include <string.h>
#include <fcntl.h>
#ifndef WIN32
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#endif // !WIN32

#include "thread.h"
#include <vector>

using namespace std;

namespace base
{
	
typedef void(*http_call_back)(struct evhttp_request *req, void *arg);
	
class COMMON_API httpserver 
    : public process_thread
{
public:
	httpserver();
	~httpserver();
	int init(string sip, int nport, http_call_back p_func, string &msg, int ntimeout=60);
	virtual void run();
	void stop();
public:
	string m_ip;
	int m_nport;
	struct event_base *m_httpbase;
	http_call_back m_p_func_call_back;
	int m_ntimeout;
};


}

#endif //_HTTP_SERVER_H_
