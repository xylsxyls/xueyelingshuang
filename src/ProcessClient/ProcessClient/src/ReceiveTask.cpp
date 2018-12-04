#include "ReceiveTask.h"
#include "../../../ProcessServer/ProcessServer/ProcessHelper.h"
#include "../../../ProcessServer/ProcessServer/HandleManager.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "ProcessClient.h"
#include "../../../ProcessServer/ProcessServer/KeyPackage.h"
#include "WorkTask.h"

ReceiveTask::ReceiveTask():
m_client(nullptr),
m_hClientReadKey(nullptr),
m_exit(false)
{
	
}

void ReceiveTask::DoTask()
{
	while (!m_exit)
	{
		m_hClientReadKey = HandleManager::instance().clientReadKeyHandle();
		::WaitForSingleObject(m_hClientReadKey, INFINITE);
		if (m_exit)
		{
			return;
		}
		//��ȡԿ��
		ProcessHelper::changeToCurrentReadKey(&(m_client->m_readKey), m_client->m_position);
		KeyPackage keyPackage = ProcessHelper::readKey(m_client->m_position, m_client->m_readKey);
		//addʧ���������һ��Կ���ڴ�
		ProcessHelper::addReadKey(m_client->m_position);
		//֪ͨ����˶�ȡ���
		::ReleaseSemaphore(HandleManager::instance().clientReadKeyEndHandle(), 1, nullptr);

		//�������������
		WorkTask* workTask = new WorkTask;
		workTask->setKey(keyPackage);
		workTask->setClient(m_client);
		std::shared_ptr<WorkTask> spWorkTask;
		spWorkTask.reset(workTask);
		CTaskThreadManager::Instance().GetThreadInterface(m_client->m_workThreadId)->PostTask(spWorkTask);
	}
}

void ReceiveTask::StopTask()
{
	m_exit = true;
	::ReleaseSemaphore(m_hClientReadKey, 1, nullptr);
}

void ReceiveTask::setClient(ProcessClient* client)
{
	m_client = client;
}
