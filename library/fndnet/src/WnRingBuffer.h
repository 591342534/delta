/// \file		WnRingBuffer.h
/// \brief		WIND环形缓冲区类的声明文件
/// \author		朱荣勋
/// \version	1.0
/// \date		2011-07-19
/// \history	2011-07-19 新建

#pragma once

/// \brief WIND环形缓冲区类
class WnRingBuffer
{
public:
	/// \brief 构造函数
	/// \param unSize 缓冲区大小
	WnRingBuffer(unsigned int unSize);
	/// \brief 析构函数
	~WnRingBuffer(void);

	/// \brief 获取写参数
	/// \param pbWrite 写指针
	/// \param unEnableSize 可写大小
	void GetWriteParam(unsigned char*& pbWrite, unsigned int& unEnableSize);

	/// \brief 获取读参数
	/// \param pbRead 读指针
	/// \param unEnableSize 可读大小
	void GetReadParam(const unsigned char*& pbRead, unsigned int& unEnableSize);

	/// \brief 更新写位置
	/// \param unWriteBytes 新写入字节数
	void UpdateWritePos(unsigned int unWriteBytes);

	/// \brief 更新读位置
	/// \param unWritedBytes 新读出字节数
	void UpdateReadPos(unsigned int unReadBytes);

private:
	// 缓冲区大小
	unsigned int m_unBufferSize;

	// 缓冲区
	unsigned char *m_pbBuffer;

	// 写入指针
	unsigned char *m_pbWrite;

	// 读取指针
	unsigned char *m_pbRead;

	// 已写入的字节数
	unsigned __int64 m_ullWriteCount;

	// 已读出的字节数
	unsigned __int64 m_ullReadCount;

	// 缓冲区锁
	boost::mutex m_oMutex;
};
