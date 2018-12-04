#include "AssignTask.h"
#include "ProcessServer.h"
#include "ProcessHelper.h"
#include "HandleManager.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "LogManager/LogManagerAPI.h"

AssignTask::AssignTask():
m_server(nullptr),
m_hReadClientKeyEnd(nullptr),
m_exit(false),
m_hAssign(nullptr),
m_readKeyIndex(0),
m_readKeyPosition(0)
{

}

void AssignTask::DoTask()
{
	while (!m_exit)
	{
		m_hAssign = HandleManager::instance().assignHandle();
		::WaitForSingleObject(m_hAssign, INFINITE);
		if (m_exit)
		{
			return;
		}
		int32_t pid = ProcessHelper::readPID(m_server->m_key, m_readKeyPosition);
		//读取钥匙，通知相关进程来取
		::ReleaseSemaphore(HandleManager::instance().clientReadKeyHandle(pid), 1, nullptr);
		m_hReadClientKeyEnd = HandleManager::instance().clientReadKeyEndHandle(pid);
		::WaitForSingleObject(m_hReadClientKeyEnd, INFINITE);
		if (m_exit)
		{
			return;
		}
		m_readKeyPosition += sizeof(KeyPackage);
		if (m_readKeyPosition == ProcessHelper::keyMemoryLength() - sizeof(int32_t))
		{
			m_readKeyPosition = 0;
			SharedMemory* key = nullptr;
			if (!m_server->m_keyList.pop(&key))
			{
				LOGERROR("pop error");
			}
			if (key->mapName() != m_server->m_key->mapName())
			{
				LOGERROR("key->mapName() != m_server->m_key->mapName()");
			}
			delete key;
			delete m_server->m_key;
			m_server->m_key = new SharedMemory(ProcessHelper::keyMapName(++m_readKeyIndex));
		}
	}
}

void AssignTask::StopTask()
{
	m_exit = true;
	::ReleaseSemaphore(m_hAssign, 1, nullptr);
	::ReleaseSemaphore(m_hReadClientKeyEnd, 1, nullptr);
}

void AssignTask::setServer(ProcessServer* server)
{
	m_server = server;
}