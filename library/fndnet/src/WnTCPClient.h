/// \file		WnTCPClient.h
/// \brief		BASE通信(TCP)客户端类的声明文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

#ifdef _WIN32
#pragma once
#endif

#include "ITCPClient.h"
#include "WnEndPoint.h"

/// \brief BASE通信(TCP)客户端类
class WnTCPClient :	public IFndNetClient, public WnEndPoint
{
public:
	/// \brief 构造函数
	/// \param oIoService IO异步处理服务
	/// \param poNotify 通报对象指针
	WnTCPClient(io_service& oIoService, IFndNetClientNotify *poNotify);
	/// \brief 析构函数
	~WnTCPClient(void);

	/// \brief 连接有心跳的TCP服务器
	/// \param szIP 服务器IP
	/// \param usPort 服务器端口
	/// \param blAuth 是否验证(暂不支持)
	/// \param unTimeOut 超时时间(单位毫秒)
	void ConnectServerWithHB(const char *szIP, unsigned short usPort, bool blAuth = false, unsigned int unTimeOut = 0);

	/// \brief 发送数据给有心跳的服务器(与ConnectServerWithHB配对使用)
	/// \param pbData 数据指针
	/// \param unDataSize 数据大小
	/// \return 是否发送成功
	bool SendDataToServerWithHB(const unsigned char *pbData, unsigned int unDataSize);

	/// \brief 连接无心跳的TCP服务器(兼容现有的服务器而设计)
	/// \param szIP 服务器IP
	/// \param usPort 服务器端口
	/// \param blAuth 是否验证(暂不支持)
	/// \param unTimeOut 超时时间(单位毫秒)
	void ConnectServerWithoutHB(const char *szIP, unsigned short usPort, bool blAuth = false, unsigned int unTimeOut = 0);

	/// \brief 发送数据给无心跳的服务器(与ConnectServerWithoutHB配对使用，兼容现有的服务器而设计)
	/// \param pbData 数据指针
	/// \param unDataSize 数据大小
	/// \return 是否发送成功
	bool SendDataToServerWithoutHB(const unsigned char *pbData, unsigned int unDataSize);

	/// \brief 与服务器断开网络连接
	void CloseSocket();

	/// \brief 连接服务器通知函数
	/// \param apConnection 发出通知的连接
	/// \param nStatus 连接状态
	void OnConnect(CONNECTION_APTR apConnection, int nStatus);

	/// \brief 连接断开的通知函数
	/// \param apConnection 发出通知的连接
	/// \param nStatus 状态码
	void OnDisconnect(CONNECTION_APTR apConnection, int nStatus);

	/// \brief 接收到数据的处理函数
	/// \param apConnection 发出通知的连接
	/// \param apPacket 收到的数据包
	void OnReceiveData(CONNECTION_APTR apConnection, const PACKET_APTR& apPacket);

	/// \brief 数据解析错误的处理函数
	/// \param apConnection 发出通知的连接
	void OnDataError(CONNECTION_APTR apConnection);

	/// \brief 清除通报对象
	void ClearNotifyObj();

protected:
	/// \brief 关闭终端
	void Close();

private:
	/// \brief 连接TCP服务器
	/// \param szIP 服务器IP
	/// \param usPort 服务器端口
	/// \param blAuth 是否验证(暂不支持)
	/// \param unTimeOut 超时时间(单位毫秒)
	void Connect(const char *szIP, unsigned short usPort, bool blAuth, unsigned int unTimeOut);

	/// \brief 发送数据给服务器
	/// \param pbData 数据指针
	/// \param unDataSize 数据大小
	/// \return 是否发送成功
	bool SendData(const unsigned char *pbData, unsigned int unDataSize);

	/// \brief 连接服务器的实际线程函数
	/// \param nStatus 连接状态
	void OnConnectThread(int nStatus);

	/// \brief 连接断开的实际线程函数
	/// \param nStatus 状态码
	void OnDisconnectThread(int nStatus);

	/// \brief 数据解析错误的线程函数
	void OnDataErrorThread();

	/// \brief 连接超时处理定时器函数
	/// \param oErrorCode 错误代码
	void OnConnectTimeOutTimer(const boost::system::error_code& oErrorCode);

	/// \brief 无接收数据定时器函数
	/// \param oErrorCode 错误代码
	void OnNoRecvDataTimer(const boost::system::error_code& oErrorCode);

private:
	/// \brief 与服务器的连接对象
	CONNECTION_APTR m_apConnection;

	/// \brief 连接超时定时器
	deadline_timer m_oTimer;

	/// \brief 接收超时定时器
	deadline_timer m_oTimeOutTimer;

	/// \brief 动作锁
	boost::mutex m_oOperateMutex;

	/// \brief 通报对象指针
	IFndNetClientNotify *m_poNotify;

	/// \brief 通报对象锁
	boost::shared_mutex m_oNotifyMutex;
};

