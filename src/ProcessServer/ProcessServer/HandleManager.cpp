#include "HandleManager.h"
#include "ProcessHelper.h"
#include "CSystem/CSystemAPI.h"

HandleManager::HandleManager()
{
	m_assignHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), "ProcessAssign");
	m_createDataHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), "ProcessCreateData");;
	m_createDataEndHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), "ProcessCreateDataEnd");;
	m_createKeyHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), "ProcessCreateKey");;
	m_createKeyEndHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), "ProcessCreateKeyEnd");;
	m_deleteDataHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), "ProcessDeleteData");;
	m_deleteDataEndHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), "ProcessDeleteDataEnd");;
}

HandleManager::~HandleManager()
{
	if (m_assignHandle)
	{
		::CloseHandle(m_assignHandle);
	}
	if (m_createDataHandle)
	{
		::CloseHandle(m_createDataHandle);
	}
	if (m_createDataEndHandle)
	{
		::CloseHandle(m_createDataEndHandle);
	}
	if (m_createKeyHandle)
	{
		::CloseHandle(m_createKeyHandle);
	}
	if (m_createKeyEndHandle)
	{
		::CloseHandle(m_createKeyEndHandle);
	}
	if (m_deleteDataHandle)
	{
		::CloseHandle(m_deleteDataHandle);
	}
	if (m_deleteDataEndHandle)
	{
		::CloseHandle(m_deleteDataEndHandle);
	}

	for (auto itClientHandle = m_clientKeyMap.begin(); itClientHandle != m_clientKeyMap.end(); ++itClientHandle)
	{
		if (itClientHandle->second)
		{
			::CloseHandle(itClientHandle->second);
		}
	}
}

HandleManager& HandleManager::instance()
{
	static HandleManager handleManager;
	return handleManager;
}

HANDLE HandleManager::assignHandle()
{
	return m_assignHandle;
}

HANDLE HandleManager::clientReadKeyHandle(int32_t pid)
{
	if (pid == 0)
	{
		pid = CSystem::processPid();
	}
	auto itClientHandle = m_clientKeyMap.find(pid);
	if (itClientHandle != m_clientKeyMap.end())
	{
		return itClientHandle->second;
	}
	HANDLE clientHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), ProcessHelper::clientReadKeyName(pid).c_str());
	m_clientKeyMap[pid] = clientHandle;
	return clientHandle;
}

HANDLE HandleManager::clientReadKeyEndHandle(int32_t pid)
{
	if (pid == 0)
	{
		pid = CSystem::processPid();
	}
	auto itClientEndHandle = m_clientKeyEndMap.find(pid);
	if (itClientEndHandle != m_clientKeyEndMap.end())
	{
		return itClientEndHandle->second;
	}
	HANDLE clientEndHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), ProcessHelper::clientReadKeyEndName(pid).c_str());
	m_clientKeyEndMap[pid] = clientEndHandle;
	return clientEndHandle;
}

HANDLE HandleManager::createDataHandle()
{
	return m_createDataHandle;
}

HANDLE HandleManager::createDataEndHandle()
{
	return m_createDataEndHandle;
}

HANDLE HandleManager::createKeyHandle()
{
	return m_createKeyHandle;
}

HANDLE HandleManager::createKeyEndHandle()
{
	return m_createKeyEndHandle;
}

HANDLE HandleManager::deleteDataHandle()
{
	return m_deleteDataHandle;
}

HANDLE HandleManager::deleteDataEndHandle()
{
	return m_deleteDataEndHandle;
}
