/// \file		WnAcceptor.h
/// \brief		BASE监听接收类的声明文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

#ifdef _WIN32
#pragma once
#endif

#include "WnAcceptorBase.h"
#include "WnConnection.h"

#ifdef _DEBUG
#define new	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

/// \brief BASE监听接收类
template<typename Protocol>
class WnAcceptor : public WnAcceptorBase, public boost::enable_shared_from_this<WnAcceptor<Protocol> >
{
public:
	typedef typename Protocol::acceptor acceptor_type;
	typedef typename Protocol::endpoint endpoint_type;
	typedef typename Protocol::resolver resolver_type;

	/// \brief 构造函数
	/// \param apEndPoint 本地终端
	/// \param oIoService IO异步处理服务
	WnAcceptor(END_POINT_APTR apEndPoint, io_service& oIoService)
		: WnAcceptorBase(apEndPoint)
		, m_oAcceptor(oIoService)
		, m_oListenEndpoint()
		, m_oResolver(oIoService)
		, m_apNewConnection()
	{}

    ~WnAcceptor(void) {}

	/// \brief 设定终端地址
	/// \param strHostName 主机名
	/// \param strPort 端口号
    int SetAddr(const string& strHostName, const string& strPort);

	/// \brief 启动监听
	/// \return 成功与否
	bool Start();

	/// \brief 停止监听
    void Stop();

private:
	/// \brief 启动监听的实际执行函数
	/// \return 成功与否
	bool StartListen();

    /// \brief 接受连接
    void BeginAccept();

    /// \brief 接受后处理函数
	void HandleAccept(const boost::system::error_code& oErrorCode);

private:
	// 接收器
	acceptor_type m_oAcceptor;

	// 监听端口
    endpoint_type m_oListenEndpoint;

	// 分解器
	resolver_type m_oResolver;

    // 最新的连接
    CONNECTION_APTR m_apNewConnection;
};

template<typename Protocol>
int WnAcceptor<Protocol>::SetAddr(const std::string& strHostName, const std::string& strPort)
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

		m_oListenEndpoint = *itEndpoint;

	} while(false);

    return nResult;
}

template<typename Protocol>
bool WnAcceptor<Protocol>::Start()
{
	bool blResult = true;

	m_oRunningMutex.lock();

	if(!m_blRunning)
	{
		m_blRunning = true;

		blResult = StartListen();

		if(!blResult)
		{
			m_blRunning = false;
		}
	}

	m_oRunningMutex.unlock();

    return blResult;
}

template<typename Protocol>
void WnAcceptor<Protocol>::Stop()
{
	m_oRunningMutex.lock();

	m_blRunning = false;
	m_oAcceptor.close();
	m_apNewConnection.reset();

	m_oRunningMutex.unlock();
}

template<typename Protocol>
bool WnAcceptor<Protocol>::StartListen()
{
	bool blResult = true;
	do
	{
		try
		{
			m_oAcceptor.open(m_oListenEndpoint.protocol());
			m_oAcceptor.set_option(typename acceptor_type::reuse_address(true));
			m_oAcceptor.bind(m_oListenEndpoint);
			m_oAcceptor.listen();
		}
		catch(std::exception&)
		{
			blResult = false;
			break;
		}

		BeginAccept();

	} while(false);

    return blResult;
}

template<typename Protocol>
void WnAcceptor<Protocol>::BeginAccept()
{
    WnConnection<Protocol>* poConnection = new WnConnection<Protocol>
		(m_apLocalEndPoint, !m_apLocalEndPoint->IsEnableHB(), m_oAcceptor.get_io_service());
    m_apNewConnection.reset(poConnection);

    m_oAcceptor.async_accept(poConnection->GetSocket(),
		boost::bind(&WnAcceptor<Protocol>::HandleAccept, this->shared_from_this(), boost::asio::placeholders::error));
}

template<typename Protocol>
void WnAcceptor<Protocol>::HandleAccept(const boost::system::error_code& oErrorCode)
{
	m_oRunningMutex.lock();

	if(m_blRunning)
	{
		if(!oErrorCode)
		{
			m_apNewConnection->OnAccept();
			BeginAccept();
		}
		else
		{
			m_oAcceptor.close();
			StartListen();
		}
	}

	m_oRunningMutex.unlock();
}
