#include "ReadWriteMutex.h"

ReadWriteMutex::ReadWriteMutex()
{

}

void ReadWriteMutex::read()
{
	m_readMutex.lock();
	if (++m_readCount == 1)
	{
		m_writeMutex.lock();
	}
	m_readMutex.unlock();
}

void ReadWriteMutex::unread()
{
	m_readMutex.lock();
	if (--m_readCount == 0)
	{
		m_writeMutex.unlock();
	}
	m_readMutex.unlock();
}

void ReadWriteMutex::write()
{
	m_writeMutex.lock();
}

void ReadWriteMutex::unwrite()
{
	m_writeMutex.unlock();
}