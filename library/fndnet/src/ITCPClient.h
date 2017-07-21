/// \file		ITCPClient.h
/// \brief		通信(TCP)客户端类的声明文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

#ifdef _WIN32
#pragma once
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

/// \brief 通信(TCP)客户端通报类
class IFndNetClientNotify
{
public:
	/*
		注意：由于线程时序的原因，有可能出现OnRawData，先于OnConnected被调用的情况，
		如有同步需求，请在回调函数内自行处理。
	*/
	/// \brief 连接服务器回调函数，由nStatus指定连接是否成功。
	virtual void OnConnected(int nStatus) = 0;

	/// \brief 从服务器接收到数据回调(回调函数应复制数据后，立即返回，否则影响接收效率，有心跳方式阻塞时间不得超过15s，否则断线)
	/// \param pbData 数据指针(函数调用后即释放，勿超范围使用，勿在函数中删除)
	/// \param unDataSize 数据大小
	virtual void OnRawData(const unsigned char* pbData, unsigned int unDataSize) = 0;

	/// \brief 从服务器接收数据异常回调
	virtual void OnServerDataError() = 0;

	/// \brief Socket连接断开回调
	virtual void OnDisconnected(int nStatus) = 0;
};

/// \brief 通信(TCP)客户端类
class IFndNetClient
{
public:
	/// \brief 连接有心跳的TCP服务器
	/// \param szIP 服务器IP
	/// \param usPort 服务器端口
	/// \param blAuth 是否验证(暂不支持)
	/// \param unTimeOut 超时时间(单位秒)
	virtual void ConnectServerWithHB(const char *szIP, unsigned short usPort, bool blAuth = false, unsigned int unTimeOut = 0) = 0;

	/// \brief 发送数据给有心跳的服务器(与ConnectServerWithHB配对使用)
	/// \param pbData 数据指针
	/// \param unDataSize 数据大小
	/// \return 是否发送成功
	virtual bool SendDataToServerWithHB(const unsigned char *pbData, unsigned int unDataSize) = 0;

	/// \brief 连接无心跳的TCP服务器
	/// \param szIP 服务器IP
	/// \param usPort 服务器端口
	/// \param blAuth 是否验证(暂不支持)
	/// \param unTimeOut 超时时间(单位秒)
	virtual void ConnectServerWithoutHB(const char *szIP, unsigned short usPort, bool blAuth = false, unsigned int unTimeOut = 0) = 0;

	/// \brief 发送数据给无心跳的服务器(与ConnectServerWithoutHB配对使用)
	/// \param pbData 数据指针
	/// \param unDataSize 数据大小
	/// \return 是否发送成功
	virtual bool SendDataToServerWithoutHB(const unsigned char *pbData, unsigned int unDataSize) = 0;

	/// \brief 与服务器断开网络连接
	virtual void CloseSocket() = 0;
};

extern "C"
{
	/// \brief 创建客户端对象
	/// \param poNotifyObj 通报对象指针
	/// \return 客户端对象指针
	IFndNetClient* NetCreateClient( IFndNetClientNotify* poNotifyObj, unsigned short usThreadCount = 0 );

	/// \brief 释放客户端对象
	/// \param poClientInstance 客户端对象指针
	void NetDeleteClient( IFndNetClient* poClientInstance );
}
