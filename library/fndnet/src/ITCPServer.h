/// \file		ITCPServer.h
/// \brief		通信(TCP)客户端类的声明文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

#ifdef _WIN32

#pragma once

#else

#include "ComHeader.h"

#endif

#ifndef _BASECCOM_ENUM_DEF_
#define _BASECCOM_ENUM_DEF_

/// \brief 连接状态枚举
enum ENU_CONNECT_STATUS
{
	E_CONNECT_SUCCESS,
	E_CONNECT_ADDRERROR,
	E_CONNECT_IOERROR,
	E_CONNECT_TIMEOUT,
};

/// \brief 断开连接状态枚举
enum ENU_DISCONNECT_STATUS
{
	E_DISCONNECT_USERASK,
	E_DISCONNECT_IOERROR,
	E_DISCONNECT_RCVTIMEOUT,
};

#endif //_BASECCOM_ENUM_DEF_

/// \brief 通信(TCP)服务端通报类
class IFndNetServerNotify
{
public:
	/*
		注意：由于线程时序的原因，有可能出现OnRawData，先于OnAccept被调用的情况，
		如有同步需求，请在回调函数内自行处理。
	*/

	/// \brief 接受客户端连接回调
	/// \param ullClientID 客户端ID
	/// \param szClientIP 客户端IP
	/// \param usPort 客户器端口
	virtual void OnAccept(unsigned __int64 ullClientID, const char *szClientIP, unsigned short usPort) = 0;

	/// \brief 从客户端接收到数据回调(回调函数应复制数据后，立即返回，否则影响接收效率，有心跳方式阻塞时间不得超过15s，否则断线)
	/// \param ullClientID 客户端ID
	/// \param pbData 数据指针(函数调用后即释放，勿超范围使用，勿在函数中删除)
	/// \param unDataSize 数据大小
	virtual void OnRawData(unsigned __int64 ullClientID, const unsigned char *pbData, unsigned int unDataSize) = 0;

	/// \brief 从客户端接收数据异常回调
	/// \param ullClientID 客户端ID
	virtual void OnClientDataError(unsigned __int64 ullClientID) = 0;

	/// \brief 断开客户端网络连接回调
	/// \param ullClientID 客户端ID
	virtual void OnClientDisconnect(unsigned __int64 ullClientID) = 0;
};

/// \brief 通信(TCP)服务端类
class IFndNetServer
{
public:
/*	/// \brief 析构函数
	virtual ~IFndNetServer() {}
*/
	/// \brief 在指定端口启动TCP服务器侦听
	/// \param usPort 监听端口号
	/// \param blAuth 是否验证(暂不支持)
	virtual bool StartListenWithHB(unsigned short usPort, bool blAuth = false) = 0;

	/// \brief 发送数据给指定的客户端
	/// \param ullClientID 客户端ID
	/// \param pbData 数据指针
	/// \param unDataSize 数据大小
	/// \return 是否发送成功
	virtual bool SendDataWithHB(unsigned __int64 ullClientID, const unsigned char *pbData, unsigned int unDataSize) = 0;

	/// \brief 在指定端口启动无心跳的TCP服务器侦听(无心跳包，仅用于兼容现有的Relay而设计)
	/// \param usPort 监听端口号
	/// \param blAuth 是否验证(暂不支持)
	virtual bool StartListenWithoutHB(unsigned short usPort, bool blAuth = false) = 0;

	/// \brief 发送数据给无心跳的客户端(与StartListen配对使用)
	/// \param ullClientID 客户端ID
	/// \param pbData 数据指针
	/// \param unDataSize 数据大小
	/// \return 是否发送成功
	virtual bool SendDataWithoutHB(unsigned __int64 ullClientID, const unsigned char *pbData, unsigned int unDataSize) = 0;

	/// \brief 关闭TCP Server
	virtual void CloseSocket() = 0;
};

extern "C"
{
	/// \brief 创建服务端对象
	/// \param poNotifyObj 通报对象指针
	/// \return 服务端对象指针(会自动删除，勿删除)
	IFndNetServer* NetCreateServer(IFndNetServerNotify* poNotifyObj, unsigned short usThreadCount = 0);

	/// \brief 释放服务端对象
	/// \param poServerInstance 服务端对象指针
	void NetDeleteServer( IFndNetServer *poServerInstance );
}
