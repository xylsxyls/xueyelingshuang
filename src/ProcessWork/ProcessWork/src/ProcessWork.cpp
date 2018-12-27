#include "ProcessWork.h"
#include "ProcessHelper.h"
#include "HandleManager.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "ReceiveTask.h"
#include "CSystem/CSystemAPI.h"
#include "ProcessMutexManager.h"
#include "SharedMemoryManager.h"
#include "ThreadManager.h"

ProcessWork::ProcessWork():
m_callback(nullptr),
m_processPid(0)
{
	m_processPid = CSystem::processPid();
}

ProcessWork& ProcessWork::instance()
{
	static ProcessWork ProcessWork;
	return ProcessWork;
}

void ProcessWork::uninit()
{
	SharedMemoryManager::instance().uninit();
	ThreadManager::instance().uninit();
	ProcessMutexManager::instance().uninit();
	HandleManager::instance().uninit();
}

void ProcessWork::initReceive(ReceiveCallback* callback)
{
	m_callback = callback;
	ReceiveTask* receiveTask = new ReceiveTask;
	receiveTask->setClient(this);
	std::shared_ptr<ReceiveTask> spReceiveTask;
	spReceiveTask.reset(receiveTask);
	ThreadManager::instance().postReceiveTask(spReceiveTask);
}

void ProcessWork::send(const char* buffer, int32_t length, int32_t pid, int32_t protocolId)
{
	void* data = nullptr;
	int32_t sendIndex = 0;
	int32_t sendBegin = 0;
	{
		WriteLock writeLock(*(ProcessMutexManager::instance().positionMutex(pid)));
		
		//�޸ķ���λ��
		if (!SharedMemoryManager::instance().addDataPosition(pid, length))
		{
			//������ɹ��������¹����ڴ�
			::ReleaseSemaphore(HandleManager::instance().createDataHandle(pid, true), 1, nullptr);
			::WaitForSingleObject(HandleManager::instance().createDataEndHandle(pid, true), INFINITE);
			//�޸ķ���λ��
			if (!SharedMemoryManager::instance().addDataPosition(pid, length))
			{
				LOGERROR("change error");
			}
		}

		//��ȡ���������ڴ棬����
		SharedMemory* sendData = SharedMemoryManager::instance().getCurrentSendData(pid);

		//�޸ĵ�ǰ�����ڴ���ʹ�ô�С
		SharedMemoryManager::instance().addDataAlreadyUsed(pid, length);
		data = sendData->writeWithoutLock();

		sendIndex = SharedMemoryManager::instance().sendDataIndex(pid);
		sendBegin = SharedMemoryManager::instance().sendDataPosition(pid) - length;
	}

	//����
	::memcpy((char*)data + sendBegin, buffer, length);

	{
		WriteLock writeLock(*(ProcessMutexManager::instance().positionMutex(pid)));
		
		//�޸�Կ��λ��
		if (!SharedMemoryManager::instance().addKeyPosition(pid))
		{
			//������ɹ��������¹����ڴ�
			::ReleaseSemaphore(HandleManager::instance().createKeyHandle(pid, true), 1, nullptr);
			::WaitForSingleObject(HandleManager::instance().createKeyEndHandle(pid, true), INFINITE);
			//�л�����ǰ
			//ProcessHelper::changeToCurrentKey(&m_key, m_position);
			//�޸�Կ��λ��
			if (!SharedMemoryManager::instance().addKeyPosition(pid))
			{
				LOGERROR("key change error");
			}
		}

		//�л�����ǰ
		//SharedMemoryManager::instance().changeToCurrentKey(pid);

		KeyPackage keyPackage;
		keyPackage.m_sendPid = m_processPid;
		keyPackage.m_receivePid = pid;
		keyPackage.m_length = length;
		keyPackage.m_protocal = protocolId;
		keyPackage.m_index = sendIndex;
		keyPackage.m_begin = sendBegin;
		//���л�����ǰԿ���ڴ���д��Կ��
		SharedMemoryManager::instance().writeKey(pid, keyPackage);
	}
	
	//֪ͨ
	::ReleaseSemaphore(HandleManager::instance().assignHandle(pid, true), 1, nullptr);
}

void ProcessWork::send(const char* buffer, int32_t length, const std::string processName, int32_t protocolId)
{
	send(buffer, length, CSystem::processPid(processName), protocolId);
}