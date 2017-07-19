/// \file		WnUtility.h
/// \brief		BASE功能类的声明文件
/// \author
/// \version	1.0
/// \date		2011-07-20
/// \history	2011-07-20 新建

#ifdef _WIN32
#pragma once
#endif

/// \brief BASE功能类
class WnUtility
{
public:
	/// \brief 获取基础IO服务对象指针
	static WnUtility* GetInstance();

	/// \brief 删除基础IO服务对象
	static void Delete();

	/// \brief 获取时间值
	/// \return 时间值
	unsigned __int64 GetTimeValue();

	/// \brief 整数转换为网络字节流
	/// \param Int 整数值
	/// \param pbStream 流指针
	template<typename Integer>
	void IntegerToStream(Integer Int, unsigned char* pbStream)
	{
		unsigned int unStreamSize = sizeof(Int);

		unsigned int unCount = 0;
		for(unCount = 0; unCount < unStreamSize; unCount++)
		{
			pbStream[unStreamSize - 1 - unCount] = Int & 0xFF;
			Int >>= 8;
		}
	}

	/// \brief 网络字节流转换为整数
	/// \param pbStream 流指针
	/// \param Int 整数值
	template<typename Integer>
	void StreamToInteger(unsigned char* pbStream, Integer& Int)
	{
		unsigned int unStreamSize = sizeof(Int);

		unsigned int unCount = 0;

		Int = 0;
		for(unCount = 0; unCount < unStreamSize; unCount++)
		{
			Int = (Int << 8) | pbStream[unCount];
		}
	}

private:
	/// \brief 构造函数
	WnUtility(void);
	/// \brief 析构函数
	~WnUtility(void);

private:
	/// \brief 功能对象的静态指针
	static WnUtility* s_poUtility;
};
