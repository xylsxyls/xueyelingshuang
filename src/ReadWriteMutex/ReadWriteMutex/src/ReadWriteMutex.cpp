#include "ReadWriteMutex.h"

ReadWriteMutex::ReadWriteMutex()
#ifdef __SUPPORT_XP__
	:m_readCount(0)
#endif
{
#ifdef _MSC_VER
#ifndef __SUPPORT_XP__
	InitializeSRWLock(&m_writeMutex);
#endif
#elif __unix__
	int result = pthread_rwlock_init(&m_writeMutex, nullptr);
	if (result != 0)
	{
		printf("pthread_rwlock_init error = %d\n", result);
	}
#endif
}

#ifdef __unix__
ReadWriteMutex::~ReadWriteMutex()
{
	int result = pthread_rwlock_destroy(&m_writeMutex);
	if (result != 0)
	{
		printf("pthread_rwlock_destroy error = %d\n", result);
	}
}
#endif

void ReadWriteMutex::read()
{
#ifdef _MSC_VER
#ifdef __SUPPORT_XP__
	std::unique_lock<std::mutex> lock(m_writeMutex);
	while (m_readCount < 0)
	{
		m_condition.wait(lock);
	}
	++m_readCount;
#else
	AcquireSRWLockShared(&m_writeMutex);
#endif
#elif __unix__
	int result = pthread_rwlock_rdlock(&m_writeMutex);
	if (result != 0)
	{
		printf("pthread_rwlock_rdlock error = %d\n", result);
	}
#endif
}

void ReadWriteMutex::unread()
{
#ifdef _MSC_VER
#ifdef __SUPPORT_XP__
	std::unique_lock<std::mutex> lock(m_writeMutex);
	if (--m_readCount == 0)
	{
		//叫醒一个等待的写操作
		m_condition.notify_one();
	}
#else
	ReleaseSRWLockShared(&m_writeMutex);
#endif
#elif __unix__
	int result = pthread_rwlock_unlock(&m_writeMutex);
	if (result != 0)
	{
		printf("pthread_rwlock_unlock error = %d\n", result);
	}
#endif
}

void ReadWriteMutex::write()
{
#ifdef _MSC_VER
#ifdef __SUPPORT_XP__
	std::unique_lock<std::mutex> lock(m_writeMutex);
	while (m_readCount != 0)
	{
		m_condition.wait(lock);
	}
	m_readCount = -1;
#else
	AcquireSRWLockExclusive(&m_writeMutex);
#endif
#elif __unix__
	int result = pthread_rwlock_wrlock(&m_writeMutex);
	if (result != 0)
	{
		printf("pthread_rwlock_wrlock error = %d\n", result);
	}
#endif
}

void ReadWriteMutex::unwrite()
{
#ifdef _MSC_VER
#ifdef __SUPPORT_XP__
	std::unique_lock<std::mutex> lock(m_writeMutex);
	m_readCount = 0;
	//叫醒所有等待的读和写操作
	m_condition.notify_all();
#else
	ReleaseSRWLockExclusive(&m_writeMutex);
#endif
#elif __unix__
	int result = pthread_rwlock_unlock(&m_writeMutex);
	if (result != 0)
	{
		printf("pthread_rwlock_unlock error = %d\n", result);
	}
#endif
}