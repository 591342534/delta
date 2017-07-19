/// \file		WnTCPServer.h
/// \brief		BASE通信(TCP)服务端类的声明文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

#ifdef _WIN32
#pragma once
#endif

#include "ComHeader.h"
#include "ITCPServer.h"
#include "WnEndPoint.h"
#include "WnAcceptor.h"

#ifdef _DEBUG
#define new	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

typedef boost::shared_ptr<deadline_timer> TIMER_APTR;

/// \brief 客户端项
struct TAG_CLIENT_ITEM
{
public:
	/// \brief 连接
	CONNECTION_APTR apConnection;

	/// \brief 心跳定时器
	TIMER_APTR apHeartBeatTimer;

	/// \brief 超时定时器
	TIMER_APTR apTimeOutTimer;

private:
	/// \brief 检查索引
	unsigned char bCheckIndex;

	/// \brief 检查索引锁
	boost::mutex oMutex;

public:
	unsigned char GetCheckIndex()
	{
		oMutex.lock();
		unsigned char bResult = bCheckIndex;
		bCheckIndex++;
		oMutex.unlock();
		return bResult;
	}

	void SetCheckIndex(unsigned char bIndex)
	{
		oMutex.lock();
		bCheckIndex = bIndex;
		oMutex.unlock();
	}
};

typedef boost::shared_ptr<TAG_CLIENT_ITEM> CLIENT_ITEM_APTR;
typedef map<unsigned __int64, CLIENT_ITEM_APTR> MAP_CLIENT_ITEM;

class WnTCPServer :	public IFndNetServer, public WnEndPoint
{
public:
	/// \brief 构造函数
	/// \param oIoService IO异步处理服务
	/// \param m_poNotify 通报对象指针
	WnTCPServer(io_service& oIoService, IFndNetServerNotify *poNotify);
	/// \brief 析构函数
	~WnTCPServer(void);

	/// \brief 在指定端口启动有心跳的TCP服务器侦听
	/// \param usPort 监听端口号
	/// \param blAuth 是否验证(暂不支持)
	bool StartListenWithHB(unsigned short usPort, bool blAuth = false);

	/// \brief 发送数据给指定的客户端
	/// \param ullClientID 客户端ID
	/// \param pbData 数据指针
	/// \param unDataSize 数据大小
	/// \return 是否发送成功
	bool SendDataWithHB(unsigned __int64 ullClientID, const unsigned char *pbData, unsigned int unDataSize);

	/// \brief 在指定端口启动无心跳的TCP服务器侦听(无心跳包，仅用于兼容现有的Relay而设计)
	/// \param usPort 监听端口号
	/// \param blAuth 是否验证(暂不支持)
	bool StartListenWithoutHB(unsigned short usPort, bool blAuth = false);

	/// \brief 发送数据给无心跳的客户端(与StartListen配对使用，仅用于兼容现有的Relay而设计)
	/// \param ullClientID 客户端ID
	/// \param pbData 数据指针
	/// \param unDataSize 数据大小
	/// \return 是否发送成功
	bool SendDataWithoutHB(unsigned __int64 ullClientID, const unsigned char *pbData, unsigned int unDataSize);

	/// \brief 关闭TCP Server
	void CloseSocket();

	/// \brief 接收到连接的实际处理函数
	/// \param apConnection 发出通知的连接
	void OnAccept(CONNECTION_APTR apConnection);

	/// \brief 连接断开的实际处理函数
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
	/// \brief 在指定端口启动TCP服务器侦听
	/// \param usPort 监听端口号
	/// \param blAuth 是否验证(暂不支持)
	bool StartListen(unsigned short usPort, bool blAuth);

	/// \brief 发送数据给客户端
	/// \param ullClientID 客户端ID
	/// \param pbData 数据指针
	/// \param unDataSize 数据大小
	/// \return 是否发送成功
	bool SendData(unsigned __int64 ullClientID, const unsigned char *pbData, unsigned int unDataSize);

	/// \brief 监听的线程函数
	void ListenThread();

	/// \brief 接收到连接通知的线程函数
	/// \param apConnection 发出通知的连接
	void OnAcceptThread(CONNECTION_APTR apConnection);

	/// \brief 连接断开的实际线程函数
	/// \param apConnection 发出通知的连接
	/// \param nStatus 状态码
	void OnDisconnectThread(CONNECTION_APTR apConnection, int nStatus);

	/// \brief 数据解析错误的线程函数
	/// \param apConnection 发出通知的连接
	void OnDataErrorThread(CONNECTION_APTR apConnection);

	/// \brief 心跳定时器函数
	/// \param apClientItem 客户端项
	/// \param oErrorCode 错误代码
	void OnHeartBeatTimer(CLIENT_ITEM_APTR apClientItem, const boost::system::error_code& oErrorCode);

	/// \brief 无接收数据定时器函数
	/// \param apConnection 发出通知的连接
	/// \param oErrorCode 错误代码
	void OnNoRecvDataTimer(CONNECTION_APTR apConnection, const boost::system::error_code& oErrorCode);

private:
	/// \brief 监听器
	ACCEPTOR_APTR m_apAcceptor;

	/// \brief 监听线程句柄
	boost::shared_ptr<boost::thread> m_apListenThread;

	/// \brief 运行状态
	int m_nRunState;

	/// \brief 有心跳的客户端连接MAP
	MAP_CLIENT_ITEM m_oClientsMap;

	/// \brief 有心跳的客户端连接锁
	boost::shared_mutex m_oSharedMutex;

	/// \brief 动作锁
	boost::mutex m_oOperateMutex;

	/// \brief 通报对象指针
	IFndNetServerNotify *m_poNotify;

	/// \brief 通报对象锁
	boost::shared_mutex m_oNotifyMutex;
};
