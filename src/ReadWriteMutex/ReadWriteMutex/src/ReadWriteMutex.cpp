#include "ReadWriteMutex.h"
#include <windows.h>

ReadWriteMutex::ReadWriteMutex():
m_mutex(nullptr)
{
	m_mutex = new SRWLOCK;
	InitializeSRWLock((SRWLOCK*)m_mutex);
}

ReadWriteMutex::~ReadWriteMutex()
{
	delete (SRWLOCK*)m_mutex;
	m_mutex = nullptr;
}

void ReadWriteMutex::read()
{
	AcquireSRWLockExclusive((SRWLOCK*)m_mutex);
}

void ReadWriteMutex::unread()
{
	ReleaseSRWLockExclusive((SRWLOCK*)m_mutex);
}

void ReadWriteMutex::write()
{
	AcquireSRWLockShared((SRWLOCK*)m_mutex);
}

void ReadWriteMutex::unwrite()
{
	ReleaseSRWLockShared((SRWLOCK*)m_mutex);
}