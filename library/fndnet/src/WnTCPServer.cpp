/// \file		WnTCPServer.cpp
/// \brief		BASE通信(TCP)服务端类的定义文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

//#include "ComHeader.h"

#include "WnTCPServer.h"

#include "WnCommGlobal.h"
#include "WnBaseIOService.h"
#include "WnUtility.h"

#ifdef _DEBUG
#define new	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

enum ENU_RUN_STATE
{
	E_WAIT,
	E_RUN,
	E_STOP,
};

WnTCPServer::WnTCPServer(io_service& oIoService, IFndNetServerNotify *poNotify)
: WnEndPoint()
, m_apAcceptor()
, m_apListenThread()
, m_nRunState(E_WAIT)
, m_oClientsMap()
, m_oSharedMutex()
, m_oOperateMutex()
, m_poNotify(poNotify)
, m_oNotifyMutex()
{
	assert(poNotify != NULL);
}

WnTCPServer::~WnTCPServer(void)
{
	CloseSocket();
}

bool WnTCPServer::StartListenWithHB(unsigned short usPort, bool blAuth)
{
	m_blHB = true;

	return StartListen(usPort, blAuth);
}

bool WnTCPServer::SendDataWithHB(unsigned __int64 ullClientID, const unsigned char *pbData, unsigned int unDataSize)
{
	return m_blHB ? SendData(ullClientID, pbData, unDataSize) : false;
}

bool WnTCPServer::StartListenWithoutHB(unsigned short usPort, bool blAuth)
{
	m_blHB = false;

	return StartListen(usPort, blAuth);
}

bool WnTCPServer::SendDataWithoutHB(unsigned __int64 ullClientID, const unsigned char *pbData, unsigned int unDataSize)
{
	return m_blHB ? false : SendData(ullClientID, pbData, unDataSize);
}

void WnTCPServer::CloseSocket()
{
	boost::mutex::scoped_lock oLock(m_oOperateMutex);

	m_nRunState = E_STOP;
	
	if(m_apListenThread)
	{
		m_apListenThread->join();
		m_apListenThread.reset();
	}

	if(m_apAcceptor)
	{
		m_apAcceptor->Stop();
		m_apAcceptor.reset();
	}

#ifdef _DEBUG_MUTEX_
	printf( "m_oSharedMutex @ %d\n", __LINE__ );
#endif
	//boost::unique_lock<boost::shared_mutex> oMapLock(m_oSharedMutex);
	boost::unique_lock<boost::shared_mutex> oMapLock(m_oSharedMutex);
	for(MAP_CLIENT_ITEM::const_iterator it = m_oClientsMap.begin(); it != m_oClientsMap.end(); ++it)
	{
		it->second->apHeartBeatTimer->cancel();
		it->second->apTimeOutTimer->cancel();
		it->second->apConnection->Close();
	}

	m_oClientsMap.clear();
}

void WnTCPServer::OnAccept(CONNECTION_APTR apConnection)
{
#ifdef _DEBUG
		printf( "OnAccept\n" );
#endif

	CLIENT_ITEM_APTR apClientItem(new TAG_CLIENT_ITEM);
	apClientItem->apConnection = apConnection;
	apClientItem->apHeartBeatTimer.reset(new deadline_timer(WnBaseIOService::GetInstance()->GetIOService()));
	apClientItem->apTimeOutTimer.reset(new deadline_timer(WnBaseIOService::GetInstance()->GetIOService()));
	apClientItem->SetCheckIndex(0);

	if(m_blHB)
	{
		// 有心跳方式下，启动心跳
		apClientItem->apHeartBeatTimer->expires_from_now(millisec(HEARTBEAT_INTERVAL));
		apClientItem->apHeartBeatTimer->async_wait
			(boost::bind(&WnTCPServer::OnHeartBeatTimer, this, apClientItem, boost::asio::placeholders::error));

		// 有心跳方式下，启动超时定时器
		apClientItem->apTimeOutTimer->expires_from_now(millisec(HEARTBEAT_INTERVAL * NO_HEARTBEAT_COUNT));
		apClientItem->apTimeOutTimer->async_wait
			(boost::bind(&WnTCPServer::OnNoRecvDataTimer, this, apClientItem->apConnection, boost::asio::placeholders::error));
	}

#ifdef _DEBUG_MUTEX_
	printf( "m_oSharedMutex @ %d", __LINE__ );
#endif
	boost::unique_lock<boost::shared_mutex> oMapLock(m_oSharedMutex);
	m_oClientsMap.insert(pair<unsigned __int64, CLIENT_ITEM_APTR>
		(reinterpret_cast<unsigned __int64>(apConnection.get()), apClientItem));

	boost::thread(boost::bind(&WnTCPServer::OnAcceptThread, this, apConnection));
}

void WnTCPServer::OnDisconnect(CONNECTION_APTR apConnection, int nStatus)
{
#ifdef _DEBUG_MUTEX_
	printf( "m_oSharedMutex @ %d", __LINE__ );
#endif
	boost::unique_lock<boost::shared_mutex> oMapLock(m_oSharedMutex);

	MAP_CLIENT_ITEM::const_iterator it = m_oClientsMap.find(reinterpret_cast<unsigned __int64>(apConnection.get()));

	if(it != m_oClientsMap.end())
	{
		if(m_blHB)
		{
			it->second->apHeartBeatTimer->cancel();
			it->second->apTimeOutTimer->cancel();
		}

		m_oClientsMap.erase(it);

		boost::thread(boost::bind(&WnTCPServer::OnDisconnectThread, this, apConnection, nStatus));
	}
}

void WnTCPServer::OnReceiveData(CONNECTION_APTR apConnection, const PACKET_APTR& apPacket)
{
#ifdef _DEBUG_MUTEX_
	printf( "m_oSharedMutex @ %d", __LINE__ );
#endif
	boost::shared_lock<boost::shared_mutex> oMapLock(m_oSharedMutex);

	MAP_CLIENT_ITEM::const_iterator it = m_oClientsMap.find(reinterpret_cast<unsigned __int64>(apConnection.get()));

	if(it != m_oClientsMap.end())
	{
		if(m_blHB)
		{
			// 取消原定时器
			it->second->apTimeOutTimer->cancel();

			// 重启定时器
			it->second->apTimeOutTimer->expires_from_now(millisec(HEARTBEAT_INTERVAL * NO_HEARTBEAT_COUNT));
			it->second->apTimeOutTimer->async_wait
				(boost::bind(&WnTCPServer::OnNoRecvDataTimer, this, it->second->apConnection, boost::asio::placeholders::error));

			if((apPacket->GetPacketData() != NULL) && !apPacket->IsHeartBeatPacket())
			{
				boost::shared_lock<boost::shared_mutex> oLock(m_oNotifyMutex);
				if(m_poNotify != NULL)
				{
					m_poNotify->OnRawData(reinterpret_cast<unsigned __int64>(apConnection.get()),
						apPacket->GetBodyData(), apPacket->GetBodyLength());
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
					m_poNotify->OnRawData(reinterpret_cast<unsigned __int64>(apConnection.get()),
						apPacket->GetPacketData(), apPacket->GetPacketLength());
				}
			}
		}
	}
}

void WnTCPServer::OnDataError(CONNECTION_APTR apConnection)
{
#ifdef _DEBUG_MUTEX_
	printf( "m_oSharedMutex @ %d", __LINE__ );
#endif
	boost::shared_lock<boost::shared_mutex> oMapLock(m_oSharedMutex);

	MAP_CLIENT_ITEM::const_iterator it = m_oClientsMap.find(reinterpret_cast<unsigned __int64>(apConnection.get()));

	if(it != m_oClientsMap.end())
	{
		if(m_blHB)
		{
			// 取消原定时器
			it->second->apTimeOutTimer->cancel();

			// 重启定时器
			it->second->apTimeOutTimer->expires_from_now(millisec(HEARTBEAT_INTERVAL * NO_HEARTBEAT_COUNT));
			it->second->apTimeOutTimer->async_wait
				(boost::bind(&WnTCPServer::OnNoRecvDataTimer, this, it->second->apConnection, boost::asio::placeholders::error));
		}

		boost::thread(boost::bind(&WnTCPServer::OnDataErrorThread, this, apConnection));
	}
}

void WnTCPServer::ClearNotifyObj()
{
	boost::unique_lock<boost::shared_mutex> oLock(m_oNotifyMutex);
	m_poNotify = NULL;
}

void WnTCPServer::Close()
{
	CloseSocket();
}

bool WnTCPServer::StartListen(unsigned short usPort, bool blAuth)
{
	bool blResult = false;

	do
	{
		CloseSocket();

		boost::mutex::scoped_lock oLock(m_oOperateMutex);

		m_apAcceptor.reset(new WnAcceptor<boost::asio::ip::tcp>
			(shared_from_this(), WnBaseIOService::GetInstance()->GetIOService()));

		if(m_apAcceptor->SetAddr("0.0.0.0", boost::lexical_cast<string>(usPort)) != 0)
		{
			break;
		}

		m_nRunState = E_WAIT;

		m_apListenThread.reset(new boost::thread(boost::bind(&WnTCPServer::ListenThread, this)));

		while(m_nRunState == E_WAIT)
		{
			boost::this_thread::sleep(millisec(NORMAL_SLEEP_TIME));
		}

		if(m_nRunState == E_RUN)
		{
			blResult = true;
		}
		else
		{
			m_apListenThread->join();
			m_apListenThread.reset();
		}

	} while(false);

	return blResult;
}

bool WnTCPServer::SendData(unsigned __int64 ullClientID, const unsigned char *pbData, unsigned int unDataSize)
{
	bool blResult = false;

	do
	{
		if((pbData == NULL) || (unDataSize == 0))
		{
			break;
		}
		else
		{
			CONNECTION_APTR apConnection;

			{
#ifdef _DEBUG_MUTEX_
				printf( "m_oSharedMutex @ %d", __LINE__ );
#endif
				boost::shared_lock<boost::shared_mutex> oMapLock(m_oSharedMutex);

				MAP_CLIENT_ITEM::const_iterator it = m_oClientsMap.find(ullClientID);

				if((it == m_oClientsMap.end()) || !it->second->apConnection)
				{
					break;
				}

				apConnection = it->second->apConnection;
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

			ENU_ASYNSEND_RESULT eResult = apConnection->AsyncSend(apPacket, false);
			if((eResult == E_AR_NOSEND) || (eResult == E_AR_OVERFLOW))
			{
				// 关闭连接
				apConnection->Close();

				// 发出通知
				OnDisconnect(apConnection, E_DISCONNECT_IOERROR);
			}

			blResult = (eResult == E_AR_SUCCESS);
		}
	} while(false);

	return blResult;
}

void WnTCPServer::ListenThread()
{
	assert(m_apAcceptor);

	if(m_apAcceptor->Start())
	{
		m_nRunState = E_RUN;
	}
	else
	{
		m_nRunState = E_STOP;
	}

	while(m_nRunState == E_RUN)
	{
		boost::this_thread::sleep(millisec(NORMAL_SLEEP_TIME));
	}
}

void WnTCPServer::OnAcceptThread(CONNECTION_APTR apConnection)
{
#ifdef _DEBUG
	printf( "OnAcceptThread\n" );
#endif

	WnConnection<boost::asio::ip::tcp> *poConnection =
		dynamic_cast<WnConnection<boost::asio::ip::tcp>*>(apConnection.get());
	if(poConnection != NULL)
	{
		WnConnection<boost::asio::ip::tcp>::endpoint_type oEndPoint = poConnection->GetRemoteEndPoint();
		boost::shared_lock<boost::shared_mutex> oLock(m_oNotifyMutex);
		if(m_poNotify != NULL)
		{
			m_poNotify->OnAccept(reinterpret_cast<unsigned __int64>(apConnection.get()),
				oEndPoint.address().to_string().c_str(), oEndPoint.port());
		}
	}
}

void WnTCPServer::OnDisconnectThread(CONNECTION_APTR apConnection, int nStatus)
{
	boost::shared_lock<boost::shared_mutex> oLock(m_oNotifyMutex);
	if(m_poNotify != NULL)
	{
		m_poNotify->OnClientDisconnect(reinterpret_cast<unsigned __int64>(apConnection.get()));
	}
}

void WnTCPServer::OnDataErrorThread(CONNECTION_APTR apConnection)
{
	boost::shared_lock<boost::shared_mutex> oLock(m_oNotifyMutex);
	if(m_poNotify != NULL)
	{
		m_poNotify->OnClientDataError(reinterpret_cast<unsigned __int64>(apConnection.get()));
	}
}

void WnTCPServer::OnHeartBeatTimer(CLIENT_ITEM_APTR apClientItem, const boost::system::error_code& oErrorCode)
{
	do
	{
		if(oErrorCode)
		{
			break;
		}

		apClientItem->apHeartBeatTimer->expires_from_now(millisec(HEARTBEAT_INTERVAL));
		apClientItem->apHeartBeatTimer->async_wait
			(boost::bind(&WnTCPServer::OnHeartBeatTimer, this, apClientItem, boost::asio::placeholders::error));

		WnUtility *poUtility = WnUtility::GetInstance();

		// 获取时间值
		unsigned __int64 ullTime = poUtility->GetTimeValue();

		// 字节转换
		unsigned char bStream[sizeof(ullTime)];
		poUtility->IntegerToStream(ullTime, bStream);

#ifdef _DEBUG_MUTEX_
		printf( "AsyncSend -- HB\n" );
#endif
		// 发送心跳包
		ENU_ASYNSEND_RESULT eResult = apClientItem->apConnection->AsyncSend
			(PACKET_APTR(new WnPacket(bStream, sizeof(ullTime), apClientItem->GetCheckIndex(), HEARTBEAT_BITFLAG)), true);

#ifdef _DEBUG_MUTEX_ //Added by Yuan.z.j
		if((eResult == E_AR_NOSEND) || (eResult == E_AR_OVERFLOW))
		{
			printf( "AsyncSend return : %d\n", eResult );

			apClientItem->apHeartBeatTimer->cancel();
			apClientItem->apTimeOutTimer->cancel();
			apClientItem->apConnection->Close();

		}
#endif
	} while(false);
}

void WnTCPServer::OnNoRecvDataTimer(CONNECTION_APTR apConnection, const boost::system::error_code& oErrorCode)
{
	do
	{
		if(oErrorCode)
		{
			break;
		}

		// 关闭连接
		apConnection->Close();

		// 发出通知
		OnDisconnect(apConnection, E_DISCONNECT_RCVTIMEOUT);
	} while(false);
}

extern "C"
{
    IFndNetServer* NetCreateServer(IFndNetServerNotify* poNotifyObj, unsigned short usThreadCount)
	{
		boost::mutex::scoped_lock oGlobalLock(WnCommGlobal::GetGlobalMutex());

		// 必须最先调用
		WnCommGlobal *poGlobalVar = WnCommGlobal::GetInstance();

		poGlobalVar->StartService(usThreadCount);

		WnTCPServer* poResult = new WnTCPServer(WnBaseIOService::GetInstance()->GetIOService(), poNotifyObj);

		poGlobalVar->AddEndPoint(poResult);

		return poResult;
	}

	void NetDeleteServer(IFndNetServer *poServerInstance)
	{
		WnTCPServer *poServer = dynamic_cast<WnTCPServer*>(poServerInstance);

		if(poServer != NULL)
		{
			poServer->ClearNotifyObj();

			// 关闭终端
			poServer->CloseSocket();

			boost::mutex::scoped_lock oGlobalLock(WnCommGlobal::GetGlobalMutex());

			WnCommGlobal *poGlobalVar = WnCommGlobal::GetInstance();

			poGlobalVar->RemoveEndPoint(poServer);

			// 全部终端移除后，全局对象析构释放资源
			if(poGlobalVar->IsEndPointListEmpty())
			{
				WnCommGlobal::Delete();
			}
		}
	}
}

