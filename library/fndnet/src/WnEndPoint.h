/// \file		WnEndPoint.h
/// \brief		BASE通信终端类的声明文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建
#ifndef WNENDPOINT_H_
#define WNENDPOINT_H_

#ifdef _WIN32
#pragma once
#endif

#include "WnPacket.h"

class WnConnBase;
typedef boost::shared_ptr<WnConnBase> CONNECTION_APTR;

class WnEndPoint : public boost::enable_shared_from_this<WnEndPoint>
{
public:
	/// \brief 构造函数
	WnEndPoint();
	/// \brief 析构函数
	virtual ~WnEndPoint(void);

	/// \brief 是否有心跳
	bool IsEnableHB() { return m_blHB; }

	/// \brief 接收到连接的处理函数
	/// \param apConnection 发出通知的连接
	virtual void OnAccept(CONNECTION_APTR apConnection) {}

	/// \brief 连接返回的处理函数
	/// \param apConnection 发出通知的连接
	/// \param nStatus 连接状态
	virtual void OnConnect(CONNECTION_APTR apConnection, int nStatus) {}

	/// \brief 连接断开的处理函数
	/// \param apConnection 发出通知的连接
	/// \param nStatus 状态码
	virtual void OnDisconnect(CONNECTION_APTR apConnection, int nStatus) = 0;

	/// \brief 接收到数据的处理函数
	/// \param apConnection 发出通知的连接
	/// \param apPacket 收到的数据包
	virtual void OnReceiveData(CONNECTION_APTR apConnection, const PACKET_APTR& apPacket) = 0;

	/// \brief 数据解析错误的处理函数
	/// \param apConnection 发出通知的连接
	virtual void OnDataError(CONNECTION_APTR apConnection) = 0;

	/// \brief 关闭终端
	virtual void Close() {}

	/// \brief 判断是否需要重连
	/// \return 重连标志
	bool IsNeedReconnect() { return m_blReconnect; }

	/// \brief 设置重连标志
	/// \param blReconnect 重连标志
	void SetReconnectFlag(bool blReconnect) { m_blReconnect = blReconnect; }

	/// \brief 获取重连间隔(单位毫秒)
	/// \return 重连间隔
	unsigned int GetReconnectInterval() { return m_unReconnectInterval; }

	/// \brief 设置重连间隔(单位毫秒)
	/// \param unReconnectInterval 重连间隔
	void SetReconnectInterval(unsigned int unReconnectInterval) { m_unReconnectInterval = unReconnectInterval; }

protected:
	/// \brief 是否启动心跳模式的标志
	bool m_blHB;

	/// \brief 重连标志
	bool m_blReconnect;

	/// \brief 重连间隔
	unsigned int m_unReconnectInterval;
};

typedef boost::shared_ptr<WnEndPoint> END_POINT_APTR;
typedef std::list<END_POINT_APTR> LIST_END_POINT;

#endif
