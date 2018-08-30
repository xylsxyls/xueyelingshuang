#include "WriteLock.h"
#include "ReadWriteMutex.h"

WriteLock::WriteLock(ReadWriteMutex& mutex)
{
	m_mutex = &mutex;
	m_mutex->write();
}

WriteLock::~WriteLock()
{
	m_mutex->unwrite();
}