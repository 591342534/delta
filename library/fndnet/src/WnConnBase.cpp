/// \file		WnConnBase.cpp
/// \brief		BASE连接基类的定义文件
/// \author
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

#include "ComHeader.h"

#include "WnConnBase.h"
#include "WnUtility.h"

#ifdef _DEBUG
#define new	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

WnConnBase::WnConnBase(END_POINT_APTR apEndPoint, bool blRawData)
: m_apLocalEndPoint(apEndPoint)
, m_blRawData(blRawData)
, m_blConnected(false)
, m_oConnectedMutex()
, m_blWriting(false)
, m_oWritingMutex()
, m_oSendPacketList()
, m_unCurrentListSize(0)
, m_oLastestSendTime(second_clock::local_time())
, m_unRecvBufferSize(RECV_BUFFER_SIZE)
, m_pbRecvBuffer(new unsigned char[m_unRecvBufferSize])
, m_tHeader()
, m_unReadBytes(0)
, m_pbBodyData(NULL)
, m_unBodySize(0)
{
}

WnConnBase::~WnConnBase(void)
{
	delete []m_pbRecvBuffer;
	delete []m_pbBodyData;
}

void WnConnBase::Close()
{
	SetConnectedState(false);

	OnClose();
}

void WnConnBase::SetConnectedState(bool blConnected)
{
	boost::unique_lock<boost::shared_mutex> oLock(m_oConnectedMutex);
	m_blConnected = blConnected;
}

bool WnConnBase::IsConnected()
{
	boost::shared_lock<boost::shared_mutex> oLock(m_oConnectedMutex);
	return m_blConnected;
}

void WnConnBase::HandleInData(CONNECTION_APTR apConnection, size_t unLength)
{
	WnUtility *poUtility = WnUtility::GetInstance();

	unsigned int unCopyLen = 0;
	unsigned int unRemainLen = 0;
	unsigned int unNeedLen = 0;
	unsigned int unCheckLen = 0;
	unsigned int unSTXLen = sizeof(STX);
	unsigned int unETXLen = sizeof(ETX);
	unsigned int unHeaderLen = sizeof(TAG_PACKET_HEADER);

	const unsigned char* pbPos = m_pbRecvBuffer;
	const unsigned char* pbEnd = m_pbRecvBuffer + unLength;

	do
	{
		// 查找STX
		if(m_unReadBytes == 0)
		{
			while(pbPos < pbEnd)
			{
				pbPos++;

				if(*(pbPos - 1) == STX)
				{
					m_unReadBytes = 1;

					break;
				}
			}
		}

		// 到达数据尾部
		if(pbPos == pbEnd)
		{
			break;
		}

		// 检查数据包开始标志
		if(m_unReadBytes == 1)
		{
			pbPos++;

			if(*(pbPos - 1) == PACKET_START_FLAG)
			{
				m_unReadBytes = 2;
				m_tHeader.bStartFlag = PACKET_START_FLAG;
			}
			else
			{
				// 无标志，重新查找包头
				m_unReadBytes = 0;

				continue;
			}
		}

		// 到达数据尾部
		if(pbPos == pbEnd)
		{
			break;
		}

		unCheckLen = unSTXLen + unHeaderLen;

		// 获取包头
		if(m_unReadBytes < unCheckLen)
		{
			unRemainLen = static_cast<unsigned int>(pbEnd - pbPos);
			unNeedLen = unCheckLen - m_unReadBytes;
			unCopyLen = (unRemainLen > unNeedLen) ?	unNeedLen : unRemainLen;

			memcpy(reinterpret_cast<unsigned char*>(&m_tHeader) + m_unReadBytes - unSTXLen, pbPos, unCopyLen);

			m_unReadBytes += unCopyLen;
			pbPos += unCopyLen;

			// 检查包头
			if(m_unReadBytes == unCheckLen)
			{
				if(m_tHeader.bHeaderLen != unHeaderLen)
				{
					// 重新查找包头
					m_unReadBytes = 0;

					// 通知数据错误
					m_apLocalEndPoint->OnDataError(apConnection);

					continue;
				}

				// 分配包体内存
				poUtility->StreamToInteger(m_tHeader.bPacketLen, m_unBodySize);
				m_pbBodyData = new unsigned char[m_unBodySize + unETXLen];
			}
		}

		// 到达数据尾部
		if(pbPos == pbEnd)
		{
			break;
		}

		unCheckLen = unSTXLen + unHeaderLen + m_unBodySize + unETXLen;

		// 获取包体
		if(m_unReadBytes < unCheckLen)
		{
			unRemainLen = static_cast<unsigned int>(pbEnd - pbPos);
			unNeedLen = unCheckLen - m_unReadBytes;
			unCopyLen = (unRemainLen > unNeedLen) ? unNeedLen : unRemainLen;

			memcpy(m_pbBodyData + m_unReadBytes - unSTXLen - unHeaderLen, pbPos, unCopyLen);

			m_unReadBytes += unCopyLen;
			pbPos += unCopyLen;

			// 检查ETX
			if(m_unReadBytes == unCheckLen)
			{
				if(m_pbBodyData[m_unBodySize] != ETX)
				{
					// 重新查找包头
					m_unReadBytes = 0;

					// 通知数据错误
					m_apLocalEndPoint->OnDataError(apConnection);

					delete []m_pbBodyData;
					m_pbBodyData = NULL;

					continue;
				}

				m_apLocalEndPoint->OnReceiveData(apConnection, PACKET_APTR
					(new WnPacket(m_tHeader, m_pbBodyData, m_unBodySize)));

				memset(&m_tHeader, 0, unHeaderLen);

				delete []m_pbBodyData;
				m_pbBodyData = NULL;

				m_unReadBytes = 0;
				m_unBodySize = 0;
			}
		}
	} while(pbPos < pbEnd);
}
