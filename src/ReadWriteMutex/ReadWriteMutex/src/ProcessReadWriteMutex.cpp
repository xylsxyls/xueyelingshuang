#include "ProcessReadWriteMutex.h"
#include <windows.h>

ProcessReadWriteMutex::ProcessReadWriteMutex(const std::string& name) :
m_hLock(nullptr)
{
	//初始化的时候不被任何线程占用
	m_hLock = ::CreateMutex(NULL, FALSE, name.c_str());
}

ProcessReadWriteMutex::~ProcessReadWriteMutex()
{
	if (m_hLock != nullptr)
	{
		::CloseHandle(m_hLock);
	}
}

void ProcessReadWriteMutex::read()
{
	if (m_hLock == nullptr)
	{
		return;
	}
	::WaitForSingleObject(m_hLock, INFINITE);
}

void ProcessReadWriteMutex::unread()
{
	if (m_hLock == nullptr)
	{
		return;
	}
	::ReleaseMutex(m_hLock);
}

void ProcessReadWriteMutex::write()
{
	if (m_hLock == nullptr)
	{
		return;
	}
	::WaitForSingleObject(m_hLock, INFINITE);
}

void ProcessReadWriteMutex::unwrite()
{
	if (m_hLock == nullptr)
	{
		return;
	}
	::ReleaseMutex(m_hLock);
}