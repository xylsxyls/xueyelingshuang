#include "CreateKeyTask.h"
#include "HandleManager.h"
#include "ProcessWork.h"
#include "SharedMemoryManager.h"

CreateKeyTask::CreateKeyTask():
m_hCreateKey(nullptr)
{

}

void CreateKeyTask::DoTask()
{
	while (!m_exit)
	{
		m_hCreateKey = HandleManager::instance().createKeyHandle(m_client->m_processPid, false);
		::WaitForSingleObject(m_hCreateKey, INFINITE);
		if (m_exit)
		{
			return;
		}
		//´´½¨Ô¿³×ÄÚ´æ
		SharedMemoryManager::instance().createKey();
		::ReleaseSemaphore(HandleManager::instance().createKeyEndHandle(m_client->m_processPid, false), 1, nullptr);
	}
}

void CreateKeyTask::StopTask()
{
	m_exit = true;
	::ReleaseSemaphore(m_hCreateKey, 1, nullptr);
}

void CreateKeyTask::setClient(ProcessWork* client)
{
	m_client = client;
}