/// \file		WnRingBuffer.cpp
/// \brief		WIND环形缓冲区类的定义文件
/// \author		朱荣勋
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

#include "ComHeader.h"

#include "WnRingBuffer.h"

#ifdef _DEBUG
#define new	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

WnRingBuffer::WnRingBuffer(unsigned int unSize)
: m_unBufferSize(unSize)
, m_pbBuffer(new unsigned char[m_unBufferSize])
, m_pbWrite(m_pbBuffer)
, m_pbRead(m_pbBuffer)
, m_ullWriteCount(0)
, m_ullReadCount(0)
, m_oMutex()
{
}

WnRingBuffer::~WnRingBuffer(void)
{
	delete []m_pbBuffer;
}

void WnRingBuffer::GetWriteParam(unsigned char*& pbWrite, unsigned int& unEnableSize)
{
	boost::mutex::scoped_lock oLock(m_oMutex);

	pbWrite = m_pbWrite;

	if(m_pbWrite > m_pbRead)
	{
		unEnableSize = m_pbBuffer + m_unBufferSize - m_pbWrite;
	}
	else if(m_pbWrite < m_pbRead)
	{
		unEnableSize = m_pbRead - m_pbWrite;
	}
	else
	{
		if(m_ullReadCount == m_ullWriteCount)
		{
			unEnableSize = m_pbBuffer + m_unBufferSize - m_pbWrite;
		}
		else
		{
			unEnableSize = 0;
		}
	}
}

void WnRingBuffer::GetReadParam(const unsigned char*& pbRead, unsigned int& unEnableSize)
{
	boost::mutex::scoped_lock oLock(m_oMutex);

	pbRead = m_pbRead;

	if(m_pbWrite > m_pbRead)
	{
		unEnableSize = m_pbWrite - m_pbRead;
	}
	else if(m_pbWrite < m_pbRead)
	{
		unEnableSize = m_pbBuffer + m_unBufferSize - m_pbRead;
	}
	else
	{
		if(m_ullReadCount == m_ullWriteCount)
		{
			unEnableSize = 0;
		}
		else
		{
			unEnableSize = m_pbBuffer + m_unBufferSize - m_pbRead;
		}
	}
}

void WnRingBuffer::UpdateWritePos(unsigned int unWriteBytes)
{
	boost::mutex::scoped_lock oLock(m_oMutex);

	m_pbWrite += unWriteBytes;
	m_ullWriteCount += unWriteBytes;

	assert(m_pbWrite <= m_pbBuffer + m_unBufferSize);

	if(m_pbWrite == m_pbBuffer + m_unBufferSize)
	{
		m_pbWrite = m_pbBuffer;
	}
}

void WnRingBuffer::UpdateReadPos(unsigned int unReadBytes)
{
	boost::mutex::scoped_lock oLock(m_oMutex);

	m_pbRead += unReadBytes;
	m_ullReadCount += unReadBytes;

	assert(m_pbRead <= m_pbBuffer + m_unBufferSize);

	if(m_pbRead == m_pbBuffer + m_unBufferSize)
	{
		m_pbRead = m_pbBuffer;
	}
}

