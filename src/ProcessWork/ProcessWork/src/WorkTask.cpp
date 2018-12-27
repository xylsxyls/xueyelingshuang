#include "WorkTask.h"
#include "ProcessHelper.h"
#include "HandleManager.h"
#include "ProcessWork.h"
#include "ReceiveCallback.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "DeleteTask.h"
#include "SharedMemoryManager.h"
#include "DeleteDataTask.h"
#include "ThreadManager.h"

WorkTask::WorkTask():
m_client(nullptr),
m_exit(false)
{

}

void WorkTask::DoTask()
{
	char* buffer = new char[m_keyPackage.m_length + 1];
	buffer[m_keyPackage.m_length] = 0;
	SharedMemoryManager::instance().readData(buffer, m_keyPackage);
	if (m_client->m_callback != nullptr)
	{
		m_client->m_callback->receive(buffer, m_keyPackage.m_length, m_keyPackage.m_sendPid, m_keyPackage.m_protocal);
	}
	delete[] buffer;
	//减少当前内存使用大小，判断是否需要删除共享内存
	if (!SharedMemoryManager::instance().reduceDataValid(m_keyPackage.m_length))
	{
		DeleteDataTask* deleteDataTask = new DeleteDataTask;
		deleteDataTask->setClient(m_client);
		deleteDataTask->setDeleteDataIndex(m_keyPackage.m_index);
		std::shared_ptr<DeleteDataTask> spDeleteDataTask;
		spDeleteDataTask.reset(deleteDataTask);
		ThreadManager::instance().postDeleteDataTask(spDeleteDataTask);
	}
}

void WorkTask::setKey(const KeyPackage& keyPackage)
{
	m_keyPackage = keyPackage;
}

void WorkTask::setClient(ProcessWork* client)
{
	m_client = client;
}