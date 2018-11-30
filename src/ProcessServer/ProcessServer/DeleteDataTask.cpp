#include "DeleteDataTask.h"
#include "ProcessServer.h"
#include "ProcessHelper.h"
#include "HandleManager.h"
#include "SharedMemory/SharedMemoryAPI.h"

DeleteDataTask::DeleteDataTask():
m_server(nullptr),
m_exit(nullptr)
{

}

void DeleteDataTask::DoTask()
{
	while (!m_exit)
	{
		m_hDeleteData = HandleManager::instance().deleteDataHandle();
		::WaitForSingleObject(m_hDeleteData, INFINITE);
		if (m_exit)
		{
			return;
		}
		int32_t deleteIndex = ProcessHelper::deleteDataIndex(m_server->m_position->readWithoutLock());
		SharedMemory* deleteMemory = nullptr;
		{
			std::unique_lock<std::mutex> mu(m_server->m_dataMutex);
			auto itDeleteData = m_server->m_dataMap.find(deleteIndex);
			if (itDeleteData != m_server->m_dataMap.end())
			{
				deleteMemory = itDeleteData->second;
				m_server->m_dataMap.erase(itDeleteData);
			}
		}
		if (deleteMemory != nullptr)
		{
			delete deleteMemory;
		}
		
		//É¾³ýÍê³É
		::ReleaseSemaphore(HandleManager::instance().deleteDataEndHandle(), 1, nullptr);
	}
}

void DeleteDataTask::StopTask()
{
	m_exit = true;
	::ReleaseSemaphore(m_hDeleteData, 1, nullptr);
}

void DeleteDataTask::setServer(ProcessServer* server)
{
	m_server = server;
}