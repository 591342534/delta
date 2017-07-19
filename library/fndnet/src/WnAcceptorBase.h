/// \file		WnAcceptorBase.h
/// \brief		BASE监听基类的声明文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

#ifdef _WIN32
#pragma once
#endif

/// \brief BASE监听基类
class WnAcceptorBase
{
public:
	/// \brief 构造函数
	WnAcceptorBase(END_POINT_APTR apLocalEndPoint)
		: m_apLocalEndPoint(apLocalEndPoint)
		, m_blRunning(false)
		, m_oRunningMutex()
	{}

	/// \brief 析构函数
    virtual ~WnAcceptorBase() {}

	/// \brief 设定终端地址
	/// \param strHostName 主机名
	/// \param strPort 端口号
    virtual int SetAddr(const string& strHostName, const string& strPort) = 0;

    /// \brief 启动监听
    virtual bool Start() = 0;

	/// \brief 停止监听
    virtual void Stop() = 0;

protected:
	// 本地终端
	END_POINT_APTR m_apLocalEndPoint;

	// 正在运行的标志
    bool m_blRunning;

	// 运行状态锁
	boost::mutex m_oRunningMutex;
};

typedef boost::shared_ptr<WnAcceptorBase> ACCEPTOR_APTR;