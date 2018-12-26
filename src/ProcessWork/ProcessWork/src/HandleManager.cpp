#include "HandleManager.h"
#include "ProcessHelper.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"

HandleManager::HandleManager():
m_pid(0)
{
	m_pid = CSystem::processPid();
	//m_assignHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), "ProcessAssign");
	//m_createDataHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), "ProcessCreateData");
	//m_createDataEndHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), "ProcessCreateDataEnd");
	//m_createKeyHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), "ProcessCreateKey");
	//m_createKeyEndHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), "ProcessCreateKeyEnd");
	//m_deleteDataHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), "ProcessDeleteData");
	//m_deleteDataEndHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), "ProcessDeleteDataEnd");
}

HandleManager::~HandleManager()
{
	//if (m_assignHandle)
	//{
	//	::CloseHandle(m_assignHandle);
	//}
	//if (m_createDataHandle)
	//{
	//	::CloseHandle(m_createDataHandle);
	//}
	//if (m_createDataEndHandle)
	//{
	//	::CloseHandle(m_createDataEndHandle);
	//}
	//if (m_createKeyHandle)
	//{
	//	::CloseHandle(m_createKeyHandle);
	//}
	//if (m_createKeyEndHandle)
	//{
	//	::CloseHandle(m_createKeyEndHandle);
	//}
	//if (m_deleteDataHandle)
	//{
	//	::CloseHandle(m_deleteDataHandle);
	//}
	//if (m_deleteDataEndHandle)
	//{
	//	::CloseHandle(m_deleteDataEndHandle);
	//}
	//
	//for (auto itClientKeyHandle = m_clientKeyMap.begin(); itClientKeyHandle != m_clientKeyMap.end(); ++itClientKeyHandle)
	//{
	//	if (itClientKeyHandle->second)
	//	{
	//		::CloseHandle(itClientKeyHandle->second);
	//	}
	//}
	//
	//for (auto itClientKeyEndHandle = m_clientKeyEndMap.begin(); itClientKeyEndHandle != m_clientKeyEndMap.end(); ++itClientKeyEndHandle)
	//{
	//	if (itClientKeyEndHandle->second)
	//	{
	//		::CloseHandle(itClientKeyEndHandle->second);
	//	}
	//}
}

HandleManager& HandleManager::instance()
{
	static HandleManager handleManager;
	return handleManager;
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

void HandleManager::createReadHandle(int32_t pid)
{
	if (m_readHandle.m_assignHandle == nullptr)
	{
		m_readHandle.m_assignHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), CStringManager::Format("ProcessAssign_%d", pid).c_str());
	}
	if (m_readHandle.m_createDataHandle == nullptr)
	{
		m_readHandle.m_createDataHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), CStringManager::Format("ProcessCreateData_%d", pid).c_str());
	}
	if (m_readHandle.m_createDataEndHandle == nullptr)
	{
		m_readHandle.m_createDataEndHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), CStringManager::Format("ProcessCreateDataEnd_%d", pid).c_str());
	}
	if (m_readHandle.m_createKeyHandle == nullptr)
	{
		m_readHandle.m_createKeyHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), CStringManager::Format("ProcessCreateKey_%d", pid).c_str());
	}
	if (m_readHandle.m_createKeyEndHandle == nullptr)
	{
		m_readHandle.m_createKeyEndHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), CStringManager::Format("ProcessCreateKeyEnd_%d", pid).c_str());
	}
	if (m_readHandle.m_deleteDataHandle == nullptr)
	{
		m_readHandle.m_deleteDataHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), CStringManager::Format("ProcessDeleteData_%d", pid).c_str());
	}
	if (m_readHandle.m_deleteDataEndHandle == nullptr)
	{
		m_readHandle.m_deleteDataEndHandle = ::CreateSemaphore(nullptr, 0, ProcessHelper::semMaxCount(), CStringManager::Format("ProcessDeleteDataEnd_%d", pid).c_str());
	}
}

HANDLE HandleManager::assignHandle(int32_t pid, bool isSend)
{
	if (isSend)
	{
		openSendHandle(pid);
		return m_sendHandleMap.find(pid)->second.m_assignHandle;
	}
	createReadHandle(pid);
	return m_readHandle.m_assignHandle;
}

HANDLE HandleManager::createDataHandle(int32_t pid, bool isSend)
{
	if (isSend)
	{
		openSendHandle(pid);
		return m_sendHandleMap.find(pid)->second.m_createDataHandle;
	}
	createReadHandle(pid);
	return m_readHandle.m_createDataHandle;
}

HANDLE HandleManager::createDataEndHandle(int32_t pid, bool isSend)
{
	if (isSend)
	{
		openSendHandle(pid);
		return m_sendHandleMap.find(pid)->second.m_createDataEndHandle;
	}
	createReadHandle(pid);
	return m_readHandle.m_createDataEndHandle;
}

HANDLE HandleManager::createKeyHandle(int32_t pid, bool isSend)
{
	if (isSend)
	{
		openSendHandle(pid);
		return m_sendHandleMap.find(pid)->second.m_createKeyHandle;
	}
	createReadHandle(pid);
	return m_readHandle.m_createKeyHandle;
}

HANDLE HandleManager::createKeyEndHandle(int32_t pid, bool isSend)
{
	if (isSend)
	{
		openSendHandle(pid);
		return m_sendHandleMap.find(pid)->second.m_createKeyEndHandle;
	}
	createReadHandle(pid);
	return m_readHandle.m_createKeyEndHandle;
}

HANDLE HandleManager::deleteDataHandle(int32_t pid, bool isSend)
{
	if (isSend)
	{
		openSendHandle(pid);
		return m_sendHandleMap.find(pid)->second.m_deleteDataHandle;
	}
	createReadHandle(pid);
	return m_readHandle.m_deleteDataHandle;
}

HANDLE HandleManager::deleteDataEndHandle(int32_t pid, bool isSend)
{
	if (isSend)
	{
		openSendHandle(pid);
		return m_sendHandleMap.find(pid)->second.m_deleteDataEndHandle;
	}
	createReadHandle(pid);
	return m_readHandle.m_deleteDataEndHandle;
}