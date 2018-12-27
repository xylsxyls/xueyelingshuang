#include "CreateDataTask.h"
#include "HandleManager.h"
#include "ProcessWork.h"
#include "SharedMemoryManager.h"

CreateDataTask::CreateDataTask():
m_hCreateData(nullptr)
{

}

void CreateDataTask::DoTask()
{
	while (!m_exit)
	{
		m_hCreateData = HandleManager::instance().createDataHandle(m_client->m_processPid, false);
		::WaitForSingleObject(m_hCreateData, INFINITE);
		if (m_exit)
		{
			return;
		}
		//创建数据内存
		SharedMemoryManager::instance().createData();
		::ReleaseSemaphore(HandleManager::instance().createDataEndHandle(m_client->m_processPid, false), 1, nullptr);
	}
}

void CreateDataTask::StopTask()
{
	m_exit = true;
	::ReleaseSemaphore(m_hCreateData, 1, nullptr);
}

void CreateDataTask::setClient(ProcessWork* client)
{
	m_client = client;
}