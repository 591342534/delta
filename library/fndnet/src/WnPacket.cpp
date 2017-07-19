/// \file		WnPacket.cpp
/// \brief		BASE数据包类的定义文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

#include "ComHeader.h"

#include "WnPacket.h"
#include "WnUtility.h"

#ifdef _DEBUG
#define new	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

WnPacket::WnPacket(const TAG_PACKET_HEADER& tHeader, const unsigned char* pbData, unsigned int unSize)
: m_pbPacketData(NULL)
, m_unPacketLength()
{
	unsigned char bHeaderSize = sizeof(TAG_PACKET_HEADER);

	// 分配整个包的内存
	m_unPacketLength = unSize + sizeof(STX) + sizeof(ETX) + bHeaderSize;
	m_pbPacketData = new unsigned char[m_unPacketLength];

	unsigned char *pbPos = m_pbPacketData;

	// STX
	*pbPos = STX;
	pbPos++;

	// 包头
	memcpy(pbPos, &tHeader, bHeaderSize);
	pbPos += bHeaderSize;

	// 包体
	memcpy(pbPos, pbData, unSize);
	pbPos += unSize;

	// ETX
	*pbPos = ETX;
}

WnPacket::WnPacket(const unsigned char* pbData, unsigned int unSize, unsigned char bCheckIndex, unsigned char bFlag)
: m_pbPacketData(NULL)
, m_unPacketLength()
{
	assert((pbData != NULL) && (unSize != 0));

	unsigned char bHeaderSize = sizeof(TAG_PACKET_HEADER);

	// 构建包头
	TAG_PACKET_HEADER tHeader;
	tHeader.bStartFlag = PACKET_START_FLAG;
	tHeader.bHeaderLen = bHeaderSize;
	tHeader.bCheckIndex = bCheckIndex;
	tHeader.bFlag = bFlag;
	WnUtility::GetInstance()->IntegerToStream(unSize, tHeader.bPacketLen);

	// 分配整个包的内存
	m_unPacketLength = unSize + sizeof(STX) + sizeof(ETX) + bHeaderSize;
	m_pbPacketData = new unsigned char[m_unPacketLength];

	unsigned char *pbPos = m_pbPacketData;

	// STX
	*pbPos = STX;
	pbPos++;

	// 包头
	memcpy(pbPos, &tHeader, bHeaderSize);
	pbPos += bHeaderSize;

	// 包体
	memcpy(pbPos, pbData, unSize);
	pbPos += unSize;

	// ETX
	*pbPos = ETX;
}

WnPacket::WnPacket(const unsigned char* pbData, unsigned int unSize)
: m_pbPacketData(NULL)
, m_unPacketLength()
{
	// 分配整个包的内存
	m_unPacketLength = unSize;
	m_pbPacketData = new unsigned char[m_unPacketLength];

	memcpy(m_pbPacketData, pbData, unSize);
}

WnPacket::~WnPacket(void)
{
	delete []m_pbPacketData;
}

const unsigned char* WnPacket::GetBodyData()
{
	unsigned char* pbResult = NULL;

	if(m_pbPacketData != NULL)
	{
		pbResult = m_pbPacketData + sizeof(STX) + sizeof(TAG_PACKET_HEADER);
	}

	return pbResult;
}

unsigned int WnPacket::GetBodyLength()
{
	unsigned int unResult = 0;

	if(m_unPacketLength != 0)
	{
		unResult = m_unPacketLength - (sizeof(STX) + sizeof(ETX) + sizeof(TAG_PACKET_HEADER));
	}

	return unResult;
}

bool WnPacket::IsHeartBeatPacket()
{
	bool blResult = false;

	if(m_pbPacketData != NULL)
	{
		TAG_PACKET_HEADER *ptHeader = reinterpret_cast<TAG_PACKET_HEADER*>(m_pbPacketData + sizeof(STX));
		blResult = ((ptHeader->bFlag & HEARTBEAT_BITFLAG) != 0);
	}

	return blResult;
}

