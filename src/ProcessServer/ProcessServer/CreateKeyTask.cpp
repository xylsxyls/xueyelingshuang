#include "CreateKeyTask.h"
#include "ProcessServer.h"
#include "ProcessHelper.h"
#include "HandleManager.h"

CreateKeyTask::CreateKeyTask():
m_server(nullptr),
m_exit(false),
m_keyIndex(0)
{

}

void CreateKeyTask::DoTask()
{
	while (!m_exit)
	{
		m_hCreateKey = HandleManager::instance().createKeyHandle();
		::WaitForSingleObject(m_hCreateKey, INFINITE);
		if (m_exit)
		{
			return;
		}
		SharedMemory* key = ProcessHelper::createKeyMemory(++m_keyIndex);
		ProcessHelper::clearCalcKey(key);
		m_server->m_keyList.push(key);
		ProcessHelper::updateKeyPosition(m_server->m_position);
		//创建完成
		::ReleaseSemaphore(HandleManager::instance().createKeyEndHandle(), 1, nullptr);
	}
}

void CreateKeyTask::StopTask()
{
	m_exit = true;
	::ReleaseSemaphore(m_hCreateKey, 1, nullptr);
}

void CreateKeyTask::setServer(ProcessServer* server)
{
	m_server = server;
}