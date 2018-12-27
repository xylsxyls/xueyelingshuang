#include "ReceiveTask.h"
#include "ProcessHelper.h"
#include "HandleManager.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "ProcessWork.h"
#include "KeyPackage.h"
#include "WorkTask.h"
#include "SharedMemoryManager.h"
#include "DeleteKeyTask.h"
#include "ThreadManager.h"

ReceiveTask::ReceiveTask():
m_client(nullptr),
m_hAssgin(nullptr),
m_exit(false)
{
	
}

void ReceiveTask::DoTask()
{
	while (!m_exit)
	{
		m_hAssgin = HandleManager::instance().assignHandle(m_client->m_processPid, false);
		::WaitForSingleObject(m_hAssgin, INFINITE);
		if (m_exit)
		{
			return;
		}
		//��ȡԿ��
		KeyPackage keyPackage = SharedMemoryManager::instance().readKey();
		//addʧ�����Զ�������һ��Կ���ڴ�
		if (!SharedMemoryManager::instance().addReadKeyPosition())
		{
			//ɾ����һ��Կ���ڴ�
			DeleteKeyTask* deleteKeyTask = new DeleteKeyTask;
			deleteKeyTask->setClient(m_client);
			std::shared_ptr<DeleteKeyTask> spDeleteKeyTask;
			spDeleteKeyTask.reset(deleteKeyTask);
			ThreadManager::instance().postDeleteKeyTask(spDeleteKeyTask);
		}

		//�������������
		WorkTask* workTask = new WorkTask;
		workTask->setKey(keyPackage);
		workTask->setClient(m_client);
		std::shared_ptr<WorkTask> spWorkTask;
		spWorkTask.reset(workTask);
		ThreadManager::instance().postWorkTask(spWorkTask);
	}
}

void ReceiveTask::StopTask()
{
	m_exit = true;
	::ReleaseSemaphore(m_hAssgin, 1, nullptr);
}

void ReceiveTask::setClient(ProcessWork* client)
{
	m_client = client;
}