/// \file		WnThreadSafeList.h
/// \brief		线程安全的list模板类的声明文件
/// \author
/// \version	1.0
/// \date		2011-04-20
/// \history	2011-04-20 新建

#ifdef _WIN32
#pragma once
#endif

#include <list>
#include <boost/thread/mutex.hpp>

#ifdef _DEBUG
#define new	new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

/// \brief 线程安全的list模板类
template <typename T>
class WnThreadSafeList
{
public:
	/// \brief 构造函数
	WnThreadSafeList() : m_oList(), m_oSharedMutex() {}

	/// \brief 析构函数
	~WnThreadSafeList()
	{
		clear();
	}

	/// \brief 取队首元素(调用前，检查队列是否为空)
	/// \return 队首元素(队列为空时，返回值未定义) 
	T front()
	{
		boost::shared_lock<boost::shared_mutex> oLock(m_oSharedMutex);
		return m_oList.front();
	}

	/// \brief 取队尾元素(调用前，检查队列是否为空)
	/// \return 队尾元素(队列为空时，返回值未定义) 
	T back()
	{
		boost::shared_lock<boost::shared_mutex> oLock(m_oSharedMutex);
		return m_oList.back();
	}

	/// \brief 弹出队首元素(调用前，检查队列是否为空)
	void pop_front()
	{
		boost::unique_lock<boost::shared_mutex> oLock(m_oSharedMutex);
		m_oList.pop_front();
	}

	/// \brief 弹出队尾元素(调用前，检查队列是否为空)
	void pop_back()
	{
		boost::unique_lock<boost::shared_mutex> oLock(m_oSharedMutex);
		m_oList.pop_back();
	}

	/// \brief 将元素加入队列头部
	/// \param t 放入队列的元素 
	void push_front(T t)
	{
		boost::unique_lock<boost::shared_mutex> oLock(m_oSharedMutex);
		m_oList.push_front(t);
	}

	/// \brief 将元素加入队列尾部
	/// \param t 放入队列的元素 
	void push_back(T t)
	{
		boost::unique_lock<boost::shared_mutex> oLock(m_oSharedMutex);
		m_oList.push_back(t);
	}

	/// \brief 判断队列是否为空
	/// \return false 队列不空，true 队列为空
	bool empty()
	{
		boost::shared_lock<boost::shared_mutex> oLock(m_oSharedMutex);
		return m_oList.empty();
	}

	/// \brief 获取队列中元素个数
	/// \return 队列中元素个数
	unsigned int size()
	{
		boost::shared_lock<boost::shared_mutex> oLock(m_oSharedMutex);
		return static_cast<unsigned int>(m_oList.size());
	}

	/// \brief 清空队列
	void clear()
	{
		boost::unique_lock<boost::shared_mutex> oLock(m_oSharedMutex);
		m_oList.clear();
	}
private:
	// 内部实际数据List
	std::list<T> m_oList;

	// 队列加锁互斥体
	boost::shared_mutex m_oSharedMutex;
};