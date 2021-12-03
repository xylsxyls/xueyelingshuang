#pragma once
#include "ReadWriteMutexBase.h"
#ifdef __unix__
#include <pthread.h>
#endif
#include <atomic>

class ReadWriteMutexAPI RecursiveReadWriteMutex : public ReadWriteMutexBase
{
public:
	RecursiveReadWriteMutex();

	~RecursiveReadWriteMutex();

public:
	virtual void read();
	virtual void write();
	virtual void unread();
	virtual void unwrite();

	int lockCount();

private:
#ifdef _MSC_VER
	CRITICAL_SECTION m_lock;
#elif __unix__
	pthread_mutex_t m_lock;
	pthread_mutexattr_t m_attr;
#endif
	std::atomic<int> m_lockCount;
};