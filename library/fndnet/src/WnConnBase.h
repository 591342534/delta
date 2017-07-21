/// \file		WnConnBase.h
/// \brief		BASE连接基类的声明文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

#ifdef _WIN32
#pragma once
#endif

#include "WnPacket.h"
#include "WnEndPoint.h"
#include "WnThreadSafeList.h"

typedef WnThreadSafeList<PACKET_APTR> SAFELIST_PACKET;

enum ENU_ASYNSEND_RESULT
{
	E_AR_SUCCESS,
	E_AR_NOCONNECT,
	E_AR_NOSEND,
	E_AR_OVERFLOW,
};

/// \brief BASE连接基类
class WnConnBase
{
public:
	/// \brief 构造函数
	/// \param apEndPoint 本地终端
	/// \param blRawData 原始数据标志
	WnConnBase(END_POINT_APTR apEndPoint, bool blRawData);
	/// \brief 析构函数
	virtual ~WnConnBase(void);

	/// \brief 设置对手方
	/// \param strHostName 对方主机名
	/// \param strPort 对方端口号
	/// \return 0 设置成功，其他值为错误代码
	virtual int SetAddr(const std::string& strHostName, const std::string& strPort) = 0;

	/// \brief 客户端开始连接函数
	virtual void BeginConnect() = 0;

	/// \brief 服务端接收到连接的响应函数
	virtual void OnAccept() = 0;

	/// \brief 异步发送函数
	/// \param oPacket 数据包
	/// \param blHighPriority 是否有高优先级
	/// \return 发送结果值
	virtual ENU_ASYNSEND_RESULT AsyncSend(const PACKET_APTR& oPacket, bool blHighPriority) = 0;

	/// \brief 关闭连接函数
	void Close();

protected:
	/// \brief 关闭连接处理函数
	virtual void OnClose() = 0;

	/// \brief 设置连接状态
	/// \param blConnected 是否连接
	void SetConnectedState(bool blConnected);

	/// \brief 是否已连接
	/// \return 连接标志
	bool IsConnected();

	/// \brief 接收到数据的处理函数
	/// \param apConnection 连接指针
	/// \param unLength 数据长度
	virtual void HandleInData(CONNECTION_APTR apConnection, size_t unLength);

protected:
	// 本地终端
	END_POINT_APTR m_apLocalEndPoint;

	// 原始数据标志(无格式)
	bool m_blRawData;

	// 是否已连接成功
	bool m_blConnected;

	// 连接成功状态锁
	boost::shared_mutex m_oConnectedMutex;

	// 正在写的标志
	bool m_blWriting;

	// 写状态锁
	boost::mutex m_oWritingMutex;

	// 发送包队列
	SAFELIST_PACKET m_oSendPacketList;

	// 当前发送包队列长度
	unsigned int m_unCurrentListSize;

	// 最后发送时间
	ptime m_oLastestSendTime;

	// 缓冲区大小
	unsigned int m_unRecvBufferSize;

	// 接收缓冲区
	unsigned char* m_pbRecvBuffer;

	/// \brief 包头
	TAG_PACKET_HEADER m_tHeader;

	/// \brief 已处理字节数
	unsigned int m_unReadBytes;

	/// \brief 包体数据指针
	unsigned char *m_pbBodyData;

	/// \brief 包体数据大小
	unsigned int m_unBodySize;
};
