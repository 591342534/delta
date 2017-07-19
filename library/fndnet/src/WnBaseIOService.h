/// \file		WnBaseIOService.h
/// \brief		BASE基础IO服务类的声明文件
/// \author
/// \version	1.0
/// \date		2011-07-20
/// \history	2011-07-20 新建

#ifdef _WIN32
#pragma once
#endif

/// \brief BASE基础IO服务类的声明文件
class WnBaseIOService
{
public:
	/// \brief 获取基础IO服务对象指针
	static WnBaseIOService* GetInstance();

	/// \brief 删除基础IO服务对象
	static void Delete();

	/// \brief 启动服务
	void StartService(unsigned short usThreadCount);

	/// \brief 停止服务
	void StopService();

	/// \brief 获取IO异步处理服务
	io_service& GetIOService() { return m_oIoService; }

private:
	/// \brief 构造函数
	WnBaseIOService(void);

	/// \brief 析构函数
	~WnBaseIOService(void);

	/// \brief 获取与CPU数匹配的线程数
	unsigned short GetThreadCount();

	/// \brief IO异步处理服务线程函数
	void ThreadsFunc();

private:
	/// \brief 服务对象的静态指针
	static WnBaseIOService* s_poService;

	/// \brief IO异步处理服务
	io_service m_oIoService;

	/// \brief IO工作状态
	boost::shared_ptr<io_service::work> m_apWork;

	/// \brief IO异步处理服务线程组
	boost::thread_group m_oIoServiceThreads;

	/// \brief 停止标志
	bool m_blStop;
};
