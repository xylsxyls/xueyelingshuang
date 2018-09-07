#include "ProcessReadWriteMutex.h"
#include <windows.h>

ProcessReadWriteMutex::ProcessReadWriteMutex():
m_hLock(nullptr)
{

}

ProcessReadWriteMutex::~ProcessReadWriteMutex()
{
	if (m_hLock != nullptr)
	{
		::CloseHandle(m_hLock);
	}
}

bool ProcessReadWriteMutex::init(const std::string& name)
{
	if (m_hLock == nullptr)
	{
		//初始化的时候不被任何线程占用
		m_hLock = ::CreateMutex(NULL, FALSE, name.c_str());
		if (m_hLock == nullptr)
		{
			return false;
		}
	}
	return true;
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