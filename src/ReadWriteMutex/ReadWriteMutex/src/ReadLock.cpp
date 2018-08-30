#include "ReadLock.h"
#include "ReadWriteMutex.h"

ReadLock::ReadLock(ReadWriteMutex& mutex)
{
	m_mutex = &mutex;
	m_mutex->read();
}

ReadLock::~ReadLock()
{
	m_mutex->unread();
}