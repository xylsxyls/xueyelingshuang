#include "WriteLock.h"
#include "ReadWriteMutexBase.h"

WriteLock::WriteLock(ReadWriteMutexBase& mutex)
{
	m_mutex = &mutex;
	m_mutex->write();
}

WriteLock::~WriteLock()
{
	m_mutex->unwrite();
}