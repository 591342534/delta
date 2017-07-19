/// \file		WnConnection.h
/// \brief		BASE连接对象类的声明文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

#ifdef _WIN32
#pragma once
#endif

#include "WnConnBase.h"

#ifdef _DEBUG
#define new	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

template<typename Protocol>
class WnConnection : public WnConnBase, public boost::enable_shared_from_this<WnConnection<Protocol> >
{
public:
	typedef typename Protocol::endpoint endpoint_type;
	typedef typename Protocol::socket socket_type;
    typedef typename Protocol::resolver resolver_type;

	/// \brief 构造函数
	/// \param apEndPoint 本地终端
	/// \param blRawData 原始数据标志
	/// \param oIoService IO异步处理服务
	WnConnection(END_POINT_APTR apEndPoint, bool blRawData, io_service& oIoService)
		: WnConnBase(apEndPoint, blRawData)
		, m_oSocket(oIoService)
		, m_oRemoteEndpoint()
		, m_oResolver(oIoService)
		, m_apPacket()
	{}

	/// \brief 析构函数
	~WnConnection(void) {}

	/// \brief 设置对手方
	/// \param strHostName 对方主机名
	/// \param strPort 对方端口号
	/// \return 0 设置成功，其他值为错误代码
	int SetAddr(const std::string& strHostName, const std::string& strPort);

	/// \brief 客户端开始连接函数
	void BeginConnect();

	/// \brief 服务端接收到连接的响应函数
	void OnAccept();

	/// \brief 异步发送函数
	/// \param oPacket 数据包
	/// \param blHighPriority 是否有高优先级
	/// \return 发送结果值
	ENU_ASYNSEND_RESULT AsyncSend(const PACKET_APTR& oPacket, bool blHighPriority);

	/// \brief 获取SOCKET
	/// \return SOCKET
	socket_type& GetSocket() { return m_oSocket; }

	/// \brief 获取对手方地址
	/// \return 对手方地址
	endpoint_type& GetRemoteEndPoint() { return m_oRemoteEndpoint; }

protected:
    /// 设置远端地址
	void SetRemoteEndpoint(const endpoint_type& oEndPoint) { m_oRemoteEndpoint = oEndPoint; }

	/// \brief 异步读函数
	void BeginRead();

	/// \brief 异步写函数
	void BeginWrite();

	/// \brief 处理连接事件
	/// \param oError 错误
	void HandleConnect(const boost::system::error_code& oErrorCode);

	/// \brief 处理数据接受事件
	/// \param oError 错误
	/// \param unTransBytes 传输的字节数
	void HandleRead(const boost::system::error_code& oErrorCode, std::size_t unTransBytes);

	/// \brief 处理数据发送事件
	/// \param oError 错误
	void HandleWrite(const boost::system::error_code& oErrorCode);

	/// \brief 关闭连接处理函数
	void OnClose();

private:
	// SOCKET对象
	socket_type m_oSocket;
	
	// 连接对手方地址
	endpoint_type m_oRemoteEndpoint;

	// 分解器
	resolver_type m_oResolver;

	// 发送包
	PACKET_APTR m_apPacket;
};

template<typename Protocol>
int WnConnection<Protocol>::SetAddr(const std::string& strHostName, const std::string& strPort)
{
	int nResult = 0;

	do
	{
		typename resolver_type::query oQuery(strHostName, strPort);

		boost::system::error_code oErrorCode = boost::asio::error::host_not_found;
		typename resolver_type::iterator itEndpoint = m_oResolver.resolve(oQuery, oErrorCode);

		if(oErrorCode)
		{
			nResult = oErrorCode.value();
			break;
		}

		SetRemoteEndpoint(*itEndpoint);

	} while(false);

    return nResult;
}

template<typename Protocol>
inline void WnConnection<Protocol>::BeginConnect()
{
    m_oSocket.async_connect(m_oRemoteEndpoint,
        boost::bind(&WnConnection<Protocol>::HandleConnect, this->shared_from_this(), boost::asio::placeholders::error));
}

template<typename Protocol>
void WnConnection<Protocol>::OnAccept()
{
	// 清空发送队列
	m_oSendPacketList.clear();

	// 设置已连接状态
	SetConnectedState(true);

	// 设置远端地址
    SetRemoteEndpoint(m_oSocket.remote_endpoint());

	// 通知连接成功
    m_apLocalEndPoint->OnAccept(this->shared_from_this());

	// 开始读
    BeginRead();
}

template<typename Protocol>
ENU_ASYNSEND_RESULT WnConnection<Protocol>::AsyncSend(const PACKET_APTR& oPacket, bool blHighPriority)
{
	ENU_ASYNSEND_RESULT eResult = E_AR_SUCCESS;

	do
	{
		if(!IsConnected())
		{
			eResult = E_AR_NOCONNECT;
			break;
		}

		if(m_blRawData)
		{
			unsigned int unNewListSize = m_oSendPacketList.size();

			// 第一次发送 或 从上次更新时间至今，已有数据发出
			if((m_unCurrentListSize == 0) || (unNewListSize < m_unCurrentListSize))
			{
				// 更新发送时间
				m_oLastestSendTime = second_clock::local_time();
			}
			else
			{
				// 发送队列一直增长
				if(second_clock::local_time() > m_oLastestSendTime + milliseconds(HEARTBEAT_INTERVAL * NO_HEARTBEAT_COUNT))
				{
					eResult = E_AR_NOSEND;
					break;
				}
			}

			// 更新队列长度值
			m_unCurrentListSize = unNewListSize + 1;

			// 发送队列积压，超过阀值的情况
			if(m_unCurrentListSize > MAX_RAWDATA_COUNT)
			{
				eResult = E_AR_OVERFLOW;
				break;
			}
		}

		m_oWritingMutex.lock();

		if(blHighPriority)
		{
			m_oSendPacketList.push_front(oPacket);
		}
		else
		{
			m_oSendPacketList.push_back(oPacket);
        }

		if(m_blWriting)
		{
			m_oWritingMutex.unlock();
			break;
		}

		m_blWriting = true;

		BeginWrite();

		m_oWritingMutex.unlock();

    } while(false);

    return eResult;
}

template<typename Protocol>
void WnConnection<Protocol>::BeginRead()
{
	boost::asio::async_read(m_oSocket, buffer(m_pbRecvBuffer, m_unRecvBufferSize), transfer_at_least(1), 
		boost::bind(&WnConnection<Protocol>::HandleRead, this->shared_from_this(),
		boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
}

template<typename Protocol>
void WnConnection<Protocol>::BeginWrite()
{
	// 取队首包
	m_apPacket = m_oSendPacketList.front();
	m_oSendPacketList.pop_front();

	boost::asio::async_write(m_oSocket, buffer(m_apPacket->GetPacketData(), m_apPacket->GetPacketLength()),
        boost::bind(&WnConnection<Protocol>::HandleWrite, this->shared_from_this(), boost::asio::placeholders::error));
}

template<typename Protocol>
void WnConnection<Protocol>::HandleConnect(const boost::system::error_code& oErrorCode)
{
    if(!oErrorCode)
    {
		// 清空发送队列
		m_oSendPacketList.clear();

		// 设置已连接状态
		SetConnectedState(true);

		// 设置远端地址
        SetRemoteEndpoint(m_oSocket.remote_endpoint());

		// 通知连接成功
        m_apLocalEndPoint->OnConnect(this->shared_from_this(), E_CONNECT_SUCCESS);

		// 开始读
        BeginRead();
    }
    else
    {
		// 通知连接失败
        m_apLocalEndPoint->OnConnect(this->shared_from_this(), E_CONNECT_IOERROR);
    }
}

template<typename Protocol>
void WnConnection<Protocol>::HandleRead(const boost::system::error_code& oErrorCode, std::size_t unTransBytes)
{
	do
	{
		if(!IsConnected())
		{
			break;
		}

		if(!oErrorCode)
		{
			// 通知接收到数据
			if(m_blRawData)
			{
				m_apLocalEndPoint->OnReceiveData(this->shared_from_this(),
					PACKET_APTR(new WnPacket(m_pbRecvBuffer, static_cast<unsigned int>(unTransBytes))));
			}
			else
			{
				HandleInData(this->shared_from_this(), unTransBytes);
			}

			// 继续读
			BeginRead();
		}
		else
		{
			boost::shared_ptr<WnConnection<Protocol> > apConnection = this->shared_from_this();

			SetConnectedState(false);

			// 关闭SOCKET
			m_oSocket.close();

			// 通知连接断开
			m_apLocalEndPoint->OnDisconnect(apConnection, E_DISCONNECT_IOERROR);
		}
	} while(false);
}

template<typename Protocol>
void WnConnection<Protocol>::HandleWrite(const boost::system::error_code& oErrorCode)
{
	do
	{
		if(!IsConnected())
		{
			m_oWritingMutex.lock();
			m_oSendPacketList.clear();
			m_oWritingMutex.unlock();
			break;
		}
		
		if(!oErrorCode)
		{
			m_oWritingMutex.lock();

			if(m_oSendPacketList.empty())
			{
				m_blWriting = false;
				m_oWritingMutex.unlock();

				break;
			}
			BeginWrite();

			m_oWritingMutex.unlock();
		}
		else
		{
			boost::shared_ptr<WnConnection<Protocol> > apConnection = this->shared_from_this();

			SetConnectedState(false);

			m_oWritingMutex.lock();
			m_oSendPacketList.clear();
			m_oWritingMutex.unlock();

			// 关闭SOCKET
			m_oSocket.close();

			// 通知连接断开
			m_apLocalEndPoint->OnDisconnect(apConnection, E_DISCONNECT_IOERROR);
		}

	} while(false);
}

template<typename Protocol>
inline void WnConnection<Protocol>::OnClose()
{
	// 关闭SOCKET
    m_oSocket.close();
}
