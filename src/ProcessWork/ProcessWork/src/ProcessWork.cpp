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
#include "CreateDataTask.h"
#include "CreateKeyTask.h"

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

	SharedMemoryManager::instance().initReceiveMemory();

	CreateKeyTask* createKeyTask = new CreateKeyTask;
	createKeyTask->setClient(this);
	std::shared_ptr<CreateKeyTask> spCreateKeyTask;
	spCreateKeyTask.reset(createKeyTask);
	ThreadManager::instance().postCreateKeyTask(spCreateKeyTask);

	CreateDataTask* createDataTask = new CreateDataTask;
	createDataTask->setClient(this);
	std::shared_ptr<CreateDataTask> spCreateDataTask;
	spCreateDataTask.reset(createDataTask);
	ThreadManager::instance().postCreateDataTask(spCreateDataTask);

	ReceiveTask* receiveTask = new ReceiveTask;
	receiveTask->setClient(this);
	std::shared_ptr<ReceiveTask> spReceiveTask;
	spReceiveTask.reset(receiveTask);
	ThreadManager::instance().postReceiveTask(spReceiveTask);
}

void ProcessWork::send(const char* buffer, int32_t length, int32_t pid, int32_t protocolId)
{
	if (pid == 0)
	{
		LOGERROR("pid == 0");
		return;
	}
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
			//������һ�������ڴ�
			if (!SharedMemoryManager::instance().raiseSendData(pid, length))
			{
				LOGERROR("raiseSendData error");
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
			//������һ��Կ���ڴ�
			if (!SharedMemoryManager::instance().raiseSendKey(pid))
			{
				LOGERROR("raiseSendKey error");
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