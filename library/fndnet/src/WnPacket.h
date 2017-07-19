/// \file		WnPacket.h
/// \brief		BASE数据包类的声明文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建
#ifndef WNPACKET_H_
#define WNPACKET_H_

#ifdef _WIN32
#pragma once
#endif

/// \brief BASE数据包头结构
struct TAG_PACKET_HEADER
{
	/// \brief 开始标志
	unsigned char bStartFlag;
	/// \brief 包头长度
	unsigned char bHeaderLen;
	/// \brief 检查索引
	unsigned char bCheckIndex;
	/// \brief 标志字节
	unsigned char bFlag;
	/// \brief 包体长度(字节流)
	unsigned char bPacketLen[4];
};

/// \brief BASE数据包类
class WnPacket
{
public:
	/// \brief 构造函数(供接收方使用，有心跳)
	/// \param tHeader 包头
	/// \param pbData 包体数据指针
	/// \param unSize 包体数据大小
	WnPacket(const TAG_PACKET_HEADER& tHeader, const unsigned char* pbData, unsigned int unSize);

	/// \brief 构造函数(供发送方使用，有心跳)
	/// \param pbData 包体数据指针
	/// \param unSize 包体数据大小
	/// \param bCheckIndex 检查索引
	/// \param bFlag 标志字节
	WnPacket(const unsigned char* pbData, unsigned int unSize, unsigned char bCheckIndex, unsigned char bFlag);

	/// \brief 构造函数(无心跳方式)
	/// \param pbData 包体数据指针
	/// \param unSize 包体数据大小
	WnPacket(const unsigned char* pbData, unsigned int unSize);

	/// \brief 析构函数
	~WnPacket(void);

	/// \brief 获取数据指针
	const unsigned char* GetPacketData() { return m_pbPacketData; }

	/// \brief 获取数据大小
	unsigned int GetPacketLength() { return m_unPacketLength; }

	/// \brief 获取包体指针
	const unsigned char* GetBodyData();

	/// \brief 获取包体大小
	unsigned int GetBodyLength();

	/// \brief 是否心跳包
	bool IsHeartBeatPacket();

private:
	// 数据指针
	unsigned char* m_pbPacketData;

	// 数据大小
	unsigned int m_unPacketLength;
};

typedef boost::shared_ptr<WnPacket> PACKET_APTR;

#endif
