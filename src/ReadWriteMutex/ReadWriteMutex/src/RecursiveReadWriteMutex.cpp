#include "RecursiveReadWriteMutex.h"

RecursiveReadWriteMutex::RecursiveReadWriteMutex():
m_lockCount(0)
{
#ifdef _MSC_VER
	InitializeCriticalSection(&m_lock);
#elif __unix__
	pthread_mutexattr_init(&m_attr);
	pthread_mutexattr_settype(&m_attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&m_lock, &m_attr);
#endif
}

RecursiveReadWriteMutex::~RecursiveReadWriteMutex()
{
#ifdef _MSC_VER
	::DeleteCriticalSection(&m_lock);
#elif __unix__
	pthread_mutex_destroy(&m_lock);
	pthread_mutexattr_destroy(&m_attr);
#endif
}

void RecursiveReadWriteMutex::read()
{
	write();
}

void RecursiveReadWriteMutex::write()
{
#ifdef _MSC_VER
	EnterCriticalSection(&m_lock);
#elif __unix__
	pthread_mutex_lock(&m_lock);
#endif
	++m_lockCount;
}

void RecursiveReadWriteMutex::unread()
{
	unwrite();
}

void RecursiveReadWriteMutex::unwrite()
{
	--m_lockCount;
#ifdef _MSC_VER
	LeaveCriticalSection(&m_lock);
#elif __unix__
	pthread_mutex_unlock(&m_lock);
#endif
}

int RecursiveReadWriteMutex::lockCount()
{
	return m_lockCount;
}