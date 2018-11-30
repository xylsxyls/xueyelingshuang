#include "CreateDataTask.h"
#include "ProcessServer.h"
#include "ProcessHelper.h"
#include "HandleManager.h"

CreateDataTask::CreateDataTask() :
m_server(nullptr),
m_exit(false),
m_hCreateData(nullptr),
m_dataIndex(0)
{

}

void CreateDataTask::DoTask()
{
	while (!m_exit)
	{
		m_hCreateData = HandleManager::instance().createDataHandle();
		::WaitForSingleObject(m_hCreateData, INFINITE);
		if (m_exit)
		{
			return;
		}
		{
			std::unique_lock<std::mutex> mu(m_server->m_dataMutex);
			++m_dataIndex;
			m_server->m_dataMap[m_dataIndex] = ProcessHelper::createDataMemory(m_dataIndex);
		}
		//创建完成
		::ReleaseSemaphore(HandleManager::instance().createDataEndHandle(), 1, nullptr);
	}
}

void CreateDataTask::StopTask()
{
	m_exit = true;
	::ReleaseSemaphore(m_hCreateData, 1, nullptr);
}

void CreateDataTask::setServer(ProcessServer* server)
{
	m_server = server;
}