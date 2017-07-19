/// \file		WnCommGlobal.h
/// \brief		BASE通信模块的全局变量类的声明文件
/// \author
/// \version	1.0
/// \date		2011-07-20
/// \history	2011-07-20 新建

#ifdef _WIN32
#pragma once
#endif

#include "WnEndPoint.h"

class WnBaseIOService;
class WnUtility;

/// \brief BASE通信模块的全局变量类
class WnCommGlobal
{
public:
	/// \brief 获取全局对象锁
	static boost::mutex& GetGlobalMutex() { return s_oGlobalMutex; }

	/// \brief 获取全局变量单例对象指针
	static WnCommGlobal* GetInstance();
	/// \brief 删除全局变量单例对象
	static void Delete();

	/// \brief 启动基础服务
	/// \param usThreadCount 服务线程数
	void StartService(unsigned short usThreadCount);

	/// \brief 添加通信终端
	/// \param poEndPoint 通信终端指针
	void AddEndPoint(WnEndPoint *poEndPoint) { m_oEndPointList.push_back(END_POINT_APTR(poEndPoint)); }

	/// \brief 删除通信终端
	/// \param poEndPoint 通信终端指针
	void RemoveEndPoint(WnEndPoint *poEndPoint);

	/// \brief 通信终端表是否为空
	/// \return true为空，false不空
	bool IsEndPointListEmpty() { return m_oEndPointList.empty(); }

private:
	/// \brief 构造函数
	WnCommGlobal(void);
	/// \brief 析构函数
	~WnCommGlobal(void);

private:
	/// \brief 全局对象锁
	static boost::mutex s_oGlobalMutex;

	/// \brief 全局变量单例对象指针
	static WnCommGlobal* s_poGlobalVar;

	/// \brief 基础IO服务的指针
	WnBaseIOService *m_poBaseIOService;

	/// \brief 功能对象
	WnUtility *m_poUtility;

	/// \brief 通信终端表
	LIST_END_POINT m_oEndPointList;
};
