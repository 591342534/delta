/// \file		WnTCPClient.cpp
/// \brief		BASE通信(TCP)客户端类的定义文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

#include "ComHeader.h"

#include "WnTCPClient.h"

#include "WnCommGlobal.h"
#include "WnBaseIOService.h"
#include "WnConnection.h"

#ifdef _DEBUG
#define new	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif


WnTCPClient::WnTCPClient(io_service& oIoService, IFndNetClientNotify *poNotify)
: WnEndPoint()
, m_apConnection()
, m_oTimer(oIoService)
, m_oTimeOutTimer(oIoService)
, m_poNotify(poNotify)
, m_oNotifyMutex()
{
}

WnTCPClient::~WnTCPClient(void)
{
}

void WnTCPClient::ConnectServerWithHB(const char *szIP, unsigned short usPort, bool blAuth, unsigned int unTimeOut)
{
	m_blHB = true;

	Connect(szIP, usPort, blAuth, unTimeOut);
}

bool WnTCPClient::SendDataToServerWithHB(const unsigned char *pbData, unsigned int unDataSize)
{
	return m_blHB ? SendData(pbData, unDataSize) : false;
}

void WnTCPClient::ConnectServerWithoutHB(const char *szIP, unsigned short usPort, bool blAuth, unsigned int unTimeOut)
{
	m_blHB = false;

	Connect(szIP, usPort, blAuth, unTimeOut);
}

bool WnTCPClient::SendDataToServerWithoutHB(const unsigned char *pbData, unsigned int unDataSize)
{
	return m_blHB ? false : SendData(pbData, unDataSize);
}

void WnTCPClient::CloseSocket()
{
	boost::mutex::scoped_lock oLock(m_oOperateMutex);

	// 关闭自动重连标志
	SetReconnectFlag(false);

	if(m_apConnection)
	{
		// 取消定时器
		m_oTimer.cancel();
		m_oTimeOutTimer.cancel();

		m_apConnection->Close();
	}
}

void WnTCPClient::OnConnect(CONNECTION_APTR apConnection, int nStatus)
{
	if(m_apConnection.get() == apConnection.get())
	{
		// 取消定时器
		m_oTimer.cancel();

		if(m_blHB && (nStatus == 0))
		{
			m_oTimeOutTimer.expires_from_now(millisec(HEARTBEAT_INTERVAL * NO_HEARTBEAT_COUNT));
			m_oTimeOutTimer.async_wait(boost::bind(&WnTCPClient::OnNoRecvDataTimer, this, boost::asio::placeholders::error));
		}

		boost::thread(boost::bind(&WnTCPClient::OnConnectThread, this, nStatus));
	}
}

void WnTCPClient::OnDisconnect(CONNECTION_APTR apConnection, int nStatus)
{
	if(m_apConnection.get() == apConnection.get())
	{
		if(m_blHB)
		{
			m_oTimeOutTimer.cancel();
		}

		boost::thread(boost::bind(&WnTCPClient::OnDisconnectThread, this, nStatus));
	}
}

void WnTCPClient::OnReceiveData(CONNECTION_APTR apConnection, const PACKET_APTR& apPacket)
{
	if(m_apConnection.get() == apConnection.get())
	{
		if(m_blHB)
		{
			// 取消原定时器
			m_oTimeOutTimer.cancel();

			// 重启定时器
			m_oTimeOutTimer.expires_from_now(millisec(HEARTBEAT_INTERVAL * NO_HEARTBEAT_COUNT));
			m_oTimeOutTimer.async_wait(boost::bind(&WnTCPClient::OnNoRecvDataTimer, this, boost::asio::placeholders::error));

			if(apPacket->GetPacketData() != NULL)
			{
				if(!apPacket->IsHeartBeatPacket())
				{
					boost::shared_lock<boost::shared_mutex> oLock(m_oNotifyMutex);
					if(m_poNotify != NULL)
					{
						// 通知接收到数据
						m_poNotify->OnRawData(apPacket->GetBodyData(), apPacket->GetBodyLength());
					}
				}
				else
				{
					// 心跳应答
					m_apConnection->AsyncSend(apPacket, true);
				}
			}
		}
		else
		{
			if(apPacket->GetPacketData() != NULL)
			{
				boost::shared_lock<boost::shared_mutex> oLock(m_oNotifyMutex);
				if(m_poNotify != NULL)
				{
					// 通知接收到数据
					m_poNotify->OnRawData(apPacket->GetPacketData(), apPacket->GetPacketLength());
				}
			}
		}
	}
}

void WnTCPClient::OnDataError(CONNECTION_APTR apConnection)
{
	if(m_apConnection.get() == apConnection.get())
	{
		if(m_blHB)
		{
			// 取消原定时器
			m_oTimeOutTimer.cancel();

			// 重启定时器
			m_oTimeOutTimer.expires_from_now(millisec(HEARTBEAT_INTERVAL * NO_HEARTBEAT_COUNT));
			m_oTimeOutTimer.async_wait(boost::bind(&WnTCPClient::OnNoRecvDataTimer, this, boost::asio::placeholders::error));
		}

		boost::thread(boost::bind(&WnTCPClient::OnDataErrorThread, this));
	}
}

void WnTCPClient::ClearNotifyObj()
{
	boost::unique_lock<boost::shared_mutex> oLock(m_oNotifyMutex);
	m_poNotify = NULL;
}

void WnTCPClient::Close()
{
	CloseSocket();
}

void WnTCPClient::Connect(const char *szIP, unsigned short usPort, bool blAuth, unsigned int unTimeOut)
{
	CloseSocket();

	boost::mutex::scoped_lock oLock(m_oOperateMutex);

	// 创建连接
	m_apConnection.reset(new WnConnection<boost::asio::ip::tcp>(shared_from_this(), !m_blHB, m_oTimer.get_io_service()));

	string strHostName;
	if(szIP != NULL)
	{
		strHostName = szIP;
	}
	string strPort = boost::lexical_cast<string>(usPort);

	// 设置地址端口
	int nResult = m_apConnection->SetAddr(strHostName, strPort);
	if(nResult == 0)
	{
		// 开始连接
		m_apConnection->BeginConnect();

		// 设定连接超时定时器
		if(unTimeOut != 0)
		{
			m_oTimer.expires_from_now(seconds(unTimeOut));
			m_oTimer.async_wait(boost::bind(&WnTCPClient::OnConnectTimeOutTimer, this, boost::asio::placeholders::error));
		}
	}
	else
	{
		OnConnect(m_apConnection, E_CONNECT_ADDRERROR);
	}
}

bool WnTCPClient::SendData(const unsigned char *pbData, unsigned int unDataSize)
{
	bool blResult = false;

	do
	{
		if(!m_apConnection)
		{
			break;
		}

		if((pbData == NULL) || (unDataSize == 0))
		{
			break;
		}

		PACKET_APTR apPacket;
		if(m_blHB)
		{
			apPacket.reset(new WnPacket(pbData, unDataSize, 0, 0));
		}
		else
		{
			apPacket.reset(new WnPacket(pbData, unDataSize));
		}
#ifdef _DEBUG
		printf( "SendData -- AsyncSend\n" );
#endif
		ENU_ASYNSEND_RESULT eResult = m_apConnection->AsyncSend(apPacket, false);
		if((eResult == E_AR_NOSEND) || (eResult == E_AR_OVERFLOW))
		{
#ifdef _DEBUG
			printf( "Exception -- Close\n" );
#endif
			// 关闭连接
			m_apConnection->Close();

			// 发出通知
			OnDisconnect(m_apConnection, E_DISCONNECT_IOERROR);
		}

		blResult = (eResult == E_AR_SUCCESS);

	} while(false);

	return blResult;
}

void WnTCPClient::OnConnectThread(int nStatus)
{
	boost::shared_lock<boost::shared_mutex> oLock(m_oNotifyMutex);
	if(m_poNotify != NULL)
	{
		m_poNotify->OnConnected(nStatus);
	}
}

void WnTCPClient::OnDisconnectThread(int nStatus)
{
	boost::shared_lock<boost::shared_mutex> oLock(m_oNotifyMutex);
	if(m_poNotify != NULL)
	{
		m_poNotify->OnDisconnected(nStatus);
	}
}

void WnTCPClient::OnDataErrorThread()
{
	boost::shared_lock<boost::shared_mutex> oLock(m_oNotifyMutex);
	if(m_poNotify != NULL)
	{
		m_poNotify->OnServerDataError();
	}
}

void WnTCPClient::OnConnectTimeOutTimer(const boost::system::error_code& oErrorCode)
{
	if(!oErrorCode && m_apConnection)
	{
		m_apConnection->Close();

		OnConnect(m_apConnection, E_CONNECT_TIMEOUT);
	}
}

void WnTCPClient::OnNoRecvDataTimer(const boost::system::error_code& oErrorCode)
{
	if(!oErrorCode && m_apConnection)
	{
		// 关闭连接
		m_apConnection->Close();

		// 发出通知
		OnDisconnect(m_apConnection, E_DISCONNECT_RCVTIMEOUT);
	}
}

extern "C"
{
	IFndNetClient* NetCreateClient(IFndNetClientNotify* poNotifyObj, unsigned short usThreadCount)
	{
		boost::mutex::scoped_lock oGlobalLock(WnCommGlobal::GetGlobalMutex());

		// 必须最先调用
		WnCommGlobal *poGlobalVar = WnCommGlobal::GetInstance();

		poGlobalVar->StartService(usThreadCount);

		WnTCPClient *poResult = new WnTCPClient(WnBaseIOService::GetInstance()->GetIOService(), poNotifyObj);

		poGlobalVar->AddEndPoint(poResult);

		return poResult;
	}

	void NetDeleteClient( IFndNetClient* poClientInstance )
	{
		WnTCPClient *poClient = dynamic_cast<WnTCPClient*>(poClientInstance);

		if(poClient != NULL)
		{
			poClient->ClearNotifyObj();

			// 关闭终端
			poClient->CloseSocket();

			boost::mutex::scoped_lock oGlobalLock(WnCommGlobal::GetGlobalMutex());

			WnCommGlobal *poGlobalVar = WnCommGlobal::GetInstance();

			poGlobalVar->RemoveEndPoint(poClient);

			// 全部终端移除后，全局对象析构释放资源
			if(poGlobalVar->IsEndPointListEmpty())
			{
				WnCommGlobal::Delete();
			}
		}
	}
}

