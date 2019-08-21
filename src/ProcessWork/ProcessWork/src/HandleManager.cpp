#include "HandleManager.h"
#include "ProcessHelper.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"

HandleManager::HandleManager():
m_pid(0)
{
	m_pid = CSystem::processPid()[0];
}

HandleManager& HandleManager::instance()
{
	static HandleManager handleManager;
	return handleManager;
}

void HandleManager::uninit()
{
	for (auto itSendHandle = m_sendHandleMap.begin(); itSendHandle != m_sendHandleMap.end(); ++itSendHandle)
	{
		auto& sendHandle = itSendHandle->second;
		if (sendHandle.m_assignHandle != nullptr)
		{
			::DeleteObject(sendHandle.m_assignHandle);
		}
		if (sendHandle.m_createDataHandle != nullptr)
		{
			::DeleteObject(sendHandle.m_createDataHandle);
		}
		if (sendHandle.m_createDataEndHandle != nullptr)
		{
			::DeleteObject(sendHandle.m_createDataEndHandle);
		}
		if (sendHandle.m_createKeyHandle != nullptr)
		{
			::DeleteObject(sendHandle.m_createKeyHandle);
		}
		if (sendHandle.m_createKeyEndHandle != nullptr)
		{
			::DeleteObject(sendHandle.m_createKeyEndHandle);
		}
		if (sendHandle.m_deleteDataHandle != nullptr)
		{
			::DeleteObject(sendHandle.m_deleteDataHandle);
		}
		if (sendHandle.m_deleteDataEndHandle != nullptr)
		{
			::DeleteObject(sendHandle.m_deleteDataEndHandle);
		}
	}
	if (m_readHandle.m_assignHandle != nullptr)
	{
		::DeleteObject(m_readHandle.m_assignHandle);
	}
	if (m_readHandle.m_createDataHandle != nullptr)
	{
		::DeleteObject(m_readHandle.m_createDataHandle);
	}
	if (m_readHandle.m_createDataEndHandle != nullptr)
	{
		::DeleteObject(m_readHandle.m_createDataEndHandle);
	}
	if (m_readHandle.m_createKeyHandle != nullptr)
	{
		::DeleteObject(m_readHandle.m_createKeyHandle);
	}
	if (m_readHandle.m_createKeyEndHandle != nullptr)
	{
		::DeleteObject(m_readHandle.m_createKeyEndHandle);
	}
	if (m_readHandle.m_deleteDataHandle != nullptr)
	{
		::DeleteObject(m_readHandle.m_deleteDataHandle);
	}
	if (m_readHandle.m_deleteDataEndHandle != nullptr)
	{
		::DeleteObject(m_readHandle.m_deleteDataEndHandle);
	}
}

void HandleManager::openSendHandle(int32_t pid)
{
	auto itSendHandle = m_sendHandleMap.find(pid);
	if (itSendHandle != m_sendHandleMap.end())
	{
		return;
	}
	SendHandlePackage sendHandlePackage;
	sendHandlePackage.m_assignHandle = ::OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, CStringManager::Format("ProcessAssign_%d", pid).c_str());
	sendHandlePackage.m_createDataHandle = ::OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, CStringManager::Format("ProcessCreateData_%d", pid).c_str());
	sendHandlePackage.m_createDataEndHandle = ::OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, CStringManager::Format("ProcessCreateDataEnd_%d", pid).c_str());
	sendHandlePackage.m_createKeyHandle = ::OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, CStringManager::Format("ProcessCreateKey_%d", pid).c_str());
	sendHandlePackage.m_createKeyEndHandle = ::OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, CStringManager::Format("ProcessCreateKeyEnd_%d", pid).c_str());
	sendHandlePackage.m_deleteDataHandle = ::OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, CStringManager::Format("ProcessDeleteData_%d", pid).c_str());
	sendHandlePackage.m_deleteDataEndHandle = ::OpenSemaphore(SEMAPHORE_ALL_ACCESS, false, CStringManager::Format("ProcessDeleteDataEnd_%d", pid).c_str());
	m_sendHandleMap[pid] = sendHandlePackage;
}

void HandleManager::createReadHandle()
{
	if (m_readHandle.m_assignHandle == nullptr)
	{
		m_readHandle.m_assignHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), CStringManager::Format("ProcessAssign_%d", m_pid).c_str());
	}
	if (m_readHandle.m_createDataHandle == nullptr)
	{
		m_readHandle.m_createDataHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), CStringManager::Format("ProcessCreateData_%d", m_pid).c_str());
	}
	if (m_readHandle.m_createDataEndHandle == nullptr)
	{
		m_readHandle.m_createDataEndHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), CStringManager::Format("ProcessCreateDataEnd_%d", m_pid).c_str());
	}
	if (m_readHandle.m_createKeyHandle == nullptr)
	{
		m_readHandle.m_createKeyHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), CStringManager::Format("ProcessCreateKey_%d", m_pid).c_str());
	}
	if (m_readHandle.m_createKeyEndHandle == nullptr)
	{
		m_readHandle.m_createKeyEndHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), CStringManager::Format("ProcessCreateKeyEnd_%d", m_pid).c_str());
	}
	if (m_readHandle.m_deleteDataHandle == nullptr)
	{
		m_readHandle.m_deleteDataHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), CStringManager::Format("ProcessDeleteData_%d", m_pid).c_str());
	}
	if (m_readHandle.m_deleteDataEndHandle == nullptr)
	{
		m_readHandle.m_deleteDataEndHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), CStringManager::Format("ProcessDeleteDataEnd_%d", m_pid).c_str());
	}
}

HANDLE HandleManager::assignHandle(int32_t pid, bool isSend)
{
	if (isSend)
	{
		openSendHandle(pid);
		return m_sendHandleMap.find(pid)->second.m_assignHandle;
	}
	createReadHandle();
	return m_readHandle.m_assignHandle;
}

HANDLE HandleManager::createDataHandle(int32_t pid, bool isSend)
{
	if (isSend)
	{
		openSendHandle(pid);
		return m_sendHandleMap.find(pid)->second.m_createDataHandle;
	}
	createReadHandle();
	return m_readHandle.m_createDataHandle;
}

HANDLE HandleManager::createDataEndHandle(int32_t pid, bool isSend)
{
	if (isSend)
	{
		openSendHandle(pid);
		return m_sendHandleMap.find(pid)->second.m_createDataEndHandle;
	}
	createReadHandle();
	return m_readHandle.m_createDataEndHandle;
}

HANDLE HandleManager::createKeyHandle(int32_t pid, bool isSend)
{
	if (isSend)
	{
		openSendHandle(pid);
		return m_sendHandleMap.find(pid)->second.m_createKeyHandle;
	}
	createReadHandle();
	return m_readHandle.m_createKeyHandle;
}

HANDLE HandleManager::createKeyEndHandle(int32_t pid, bool isSend)
{
	if (isSend)
	{
		openSendHandle(pid);
		return m_sendHandleMap.find(pid)->second.m_createKeyEndHandle;
	}
	createReadHandle();
	return m_readHandle.m_createKeyEndHandle;
}

HANDLE HandleManager::deleteDataHandle(int32_t pid, bool isSend)
{
	if (isSend)
	{
		openSendHandle(pid);
		return m_sendHandleMap.find(pid)->second.m_deleteDataHandle;
	}
	createReadHandle();
	return m_readHandle.m_deleteDataHandle;
}

HANDLE HandleManager::deleteDataEndHandle(int32_t pid, bool isSend)
{
	if (isSend)
	{
		openSendHandle(pid);
		return m_sendHandleMap.find(pid)->second.m_deleteDataEndHandle;
	}
	createReadHandle();
	return m_readHandle.m_deleteDataEndHandle;
}