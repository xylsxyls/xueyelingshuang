#include "ReadWriteMutex.h"
#include <windows.h>

ReadWriteMutex::ReadWriteMutex():
m_mutex(nullptr)
{
	m_mutex = new SRWLOCK;
	InitializeSRWLock(m_mutex);
}

ReadWriteMutex::~ReadWriteMutex()
{
	delete (SRWLOCK*)m_mutex;
	m_mutex = nullptr;
}

void ReadWriteMutex::read()
{
	AcquireSRWLockExclusive(m_mutex);
}

void ReadWriteMutex::unread()
{
	ReleaseSRWLockExclusive(m_mutex);
}

void ReadWriteMutex::write()
{
	AcquireSRWLockShared(m_mutex);
}

void ReadWriteMutex::unwrite()
{
	ReleaseSRWLockShared(m_mutex);
}