/// \file		WnBaseIOService.cpp
/// \brief		BASE基础IO服务类的定义文件
/// \author
/// \version	1.0
/// \date		2011-07-20
/// \history	2011-07-20 新建

#include "ComHeader.h"

#include "WnBaseIOService.h"

#ifdef _DEBUG
#define new	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

WnBaseIOService* WnBaseIOService::s_poService = NULL;

WnBaseIOService::WnBaseIOService(void)
: m_oIoService()
, m_apWork()
, m_oIoServiceThreads()
, m_blStop(false)
{
}

WnBaseIOService::~WnBaseIOService(void)
{
}

WnBaseIOService* WnBaseIOService::GetInstance()
{
	if(s_poService == NULL)
	{
		// 创建实例
		s_poService = new WnBaseIOService;
	}

	return s_poService;
}

void WnBaseIOService::Delete()
{
	delete s_poService;
	s_poService = NULL;
}

void WnBaseIOService::StartService(unsigned short usThreadCount)
{
	if(!m_apWork)
	{
		m_blStop = false;

		// 使IO服务保持在工作状态，防止run直接返回
		m_apWork.reset(new io_service::work(m_oIoService));
	}

	// 获取合适的线程数
	if(usThreadCount < DEFAULT_IOTHREAD_COUNT)
	{
		usThreadCount = GetThreadCount();
	}

	// 创建线程组
	size_t usCount = 0;
	for(usCount = m_oIoServiceThreads.size(); usCount < usThreadCount; usCount++)
	{
		m_oIoServiceThreads.create_thread(boost::bind(&WnBaseIOService::ThreadsFunc, this));
	}
}

void WnBaseIOService::StopService()
{
	if(m_apWork)
	{
		m_blStop = true;

		// 使IO服务退出
		m_apWork.reset();

		// 等待所有线程结束
		m_oIoServiceThreads.join_all();
	}
}


unsigned short WnBaseIOService::GetThreadCount()
{
	// 获取cpu数量
	unsigned short usThreadCount = boost::thread::hardware_concurrency();

	// 获取失败
	if(usThreadCount == 0)
	{
		// 使用默认线程数
		usThreadCount = DEFAULT_IOTHREAD_COUNT;
	}
	else
	{
		// 使用推荐线程数
		usThreadCount = usThreadCount * 2 + 2;
	}

	return usThreadCount;
}

void WnBaseIOService::ThreadsFunc()
{
	// 非退出状态
	while(!m_blStop)
	{
		try
		{
			// IO异步处理服务复位
			m_oIoService.reset();

			boost::system::error_code oErrorCode;
			m_oIoService.run(oErrorCode);
			if(oErrorCode)
			{
				//?? 输出错误消息
			}
			oErrorCode.clear();
		}
		catch(boost::system::system_error const& ex)
		{
			//?? 输出错误消息

			ex.what();
		}
		catch(...)
		{
			//?? 输出错误消息
        }
    }
}
