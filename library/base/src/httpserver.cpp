/*****************************************************************************
 VBase Copyright (c) 2015. All Rights Reserved.

 FileName: httpserver.h
 Version: 1.0
 Date: 2015.10.11

 History:
 ericsheng     2015.10.11   1.0     Create
 ******************************************************************************/
 
#include "httpserver.h"
 
namespace base
{
	 
httpserver::httpserver()
{
	m_ip = "0.0.0.0";
	m_nport = 8080;
	m_httpbase = NULL;
	m_p_func_call_back = NULL;
	m_ntimeout = 60;
}

httpserver::~httpserver()
{
	m_ip = "0.0.0.0";
	m_nport = 8080;
	m_httpbase = NULL;
	m_p_func_call_back = NULL;
	m_ntimeout = 60;
}

int httpserver::init(string sip, int nport, http_call_back p_func, string &msg, int ntimeout)
{
	msg = "";
	m_ip = sip;
	m_nport = nport;
	m_p_func_call_back = p_func;
	m_httpbase = event_base_new();
	m_ntimeout = ntimeout;
	if(m_httpbase == NULL) {
		msg = "httpserver::init new http base failed";
		return -1;
	}
	
	return 0;
}

void httpserver::run()
{
	struct evhttp *http;
	struct evhttp_bound_socket *handle;
	
	
	http = evhttp_new(m_httpbase);
	if(http == NULL) {
		return;
	}
	
	evhttp_set_gencb(http,m_p_func_call_back,NULL);
	
	handle = evhttp_bind_socket_with_handle(http,m_ip.c_str(),m_nport);
    if (handle == NULL) return;
	
	evhttp_set_timeout(http,m_ntimeout);
	
	event_base_dispatch(m_httpbase);
	evhttp_free(http);
	event_base_free(m_httpbase);
	m_httpbase = 0;
	return;
}

void httpserver::stop()
{
    if (m_httpbase){
        event_base_loopbreak(m_httpbase);
    }
}
}
