/// \file		WnCommGlobal.cpp
/// \brief		BASE通信模块的全局变量类的定义文件
/// \author
/// \version	1.0
/// \date		2011-07-20
/// \history	2011-07-20 新建

#include "ComHeader.h"

#include "WnCommGlobal.h"
#include "WnBaseIOService.h"
#include "WnUtility.h"

#ifdef _DEBUG
#define new	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

boost::mutex WnCommGlobal::s_oGlobalMutex;
WnCommGlobal* WnCommGlobal::s_poGlobalVar = NULL;

WnCommGlobal::WnCommGlobal(void)
: m_poBaseIOService(WnBaseIOService::GetInstance())
, m_poUtility(WnUtility::GetInstance())
, m_oEndPointList()
{
}

WnCommGlobal::~WnCommGlobal(void)
{
	for(LIST_END_POINT::const_iterator it = m_oEndPointList.begin(); it != m_oEndPointList.end(); ++it)
	{
		it->get()->Close();
	}

	m_oEndPointList.clear();

	m_poBaseIOService->StopService();
	WnBaseIOService::Delete();

	WnUtility::Delete();
}

WnCommGlobal* WnCommGlobal::GetInstance()
{
	if(s_poGlobalVar == NULL)
	{
		s_poGlobalVar = new WnCommGlobal;
	}

	return s_poGlobalVar;
}

void WnCommGlobal::Delete()
{
	delete s_poGlobalVar;
	s_poGlobalVar = NULL;
}

void WnCommGlobal::StartService(unsigned short usThreadCount)
{
	m_poBaseIOService->StartService(usThreadCount);
}

void WnCommGlobal::RemoveEndPoint(WnEndPoint *poEndPoint)
{
	for( auto it = m_oEndPointList.begin(); it != m_oEndPointList.end(); ++it)
	{
		if(it->get() == poEndPoint)
		{
			m_oEndPointList.erase(it);
			break;
		}
	}
}
