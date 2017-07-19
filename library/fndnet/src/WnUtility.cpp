/// \file		WnUtility.cpp
/// \brief		BASE功能类的定义文件
/// \author
/// \version	1.0
/// \date		2011-07-20
/// \history	2011-07-20 新建

#include "ComHeader.h"

#include "WnUtility.h"

#ifdef _DEBUG
#define new	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

WnUtility* WnUtility::s_poUtility = NULL;

WnUtility::WnUtility(void)
{
}

WnUtility::~WnUtility(void)
{
}

WnUtility* WnUtility::GetInstance()
{
	if(s_poUtility == NULL)
	{
		// 创建实例
		s_poUtility = new WnUtility;
	}

	return s_poUtility;
}

void WnUtility::Delete()
{
	delete s_poUtility;
	s_poUtility = NULL;
}

unsigned __int64 WnUtility::GetTimeValue()
{
	// 取当前时间字符串(YYYYMMDDThhmmss)
	string strTime(to_iso_string(second_clock::local_time()));
	// 删除T字符
	strTime.erase(strTime.find('T'), 1);
	// 删除年份的前两位
	strTime.erase(0, 2);

	// 转为数字
	return boost::lexical_cast<unsigned __int64>(strTime);
}
