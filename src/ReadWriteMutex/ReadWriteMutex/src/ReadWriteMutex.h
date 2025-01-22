#pragma once
#include "ReadWriteMutexBase.h"
#include <mutex>

//#define __SUPPORT_XP__

#ifdef _MSC_VER
#include <windows.h>
#ifdef __SUPPORT_XP__
#include <atomic>
#include <condition_variable>
#endif
#elif __unix__
#include <pthread.h>
#endif

class ReadWriteMutexAPI ReadWriteMutex : public ReadWriteMutexBase
{
public:
	ReadWriteMutex();

#ifdef __unix__
	virtual ~ReadWriteMutex();
#endif

public:
	virtual void read();
	virtual void unread();
	virtual void write();
	virtual void unwrite();

private:
#ifdef _MSC_VER
	
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#ifdef __SUPPORT_XP__
	//已加读锁个数
	std::atomic<int32_t> m_readCount;
	std::mutex m_writeMutex;
	std::condition_variable m_condition;
#else
	SRWLOCK m_writeMutex;
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#elif __unix__
	pthread_rwlock_t m_writeMutex;
#endif
};