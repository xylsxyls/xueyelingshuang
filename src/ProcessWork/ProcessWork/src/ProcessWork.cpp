#include "ProcessWork.h"
#include "ProcessHelper.h"
#include "HandleManager.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "ReceiveTask.h"
#include "CSystem/CSystemAPI.h"
#include "ProcessMutexManager.h"
#include "SharedMemoryManager.h"

ProcessWork::ProcessWork():
m_position(nullptr),
m_data(nullptr),
m_key(nullptr),
m_readData(nullptr),
m_readKey(nullptr),
m_positionMutex(nullptr),
m_receiveThreadId(0),
m_workThreadId(0),
m_deleteThreadId(0),
m_callback(nullptr),
m_processPid(0),
m_deleteKeyThreadId(0),
m_deleteDataThreadId(0),
m_createKeyThreadId(0),
m_createDataThreadId(0)
{
	m_processPid = CSystem::processPid();
	m_receiveThreadId = CTaskThreadManager::Instance().Init();
	m_workThreadId = CTaskThreadManager::Instance().Init();
	m_deleteKeyThreadId = CTaskThreadManager::Instance().Init();
	m_deleteDataThreadId = CTaskThreadManager::Instance().Init();
	m_createKeyThreadId = CTaskThreadManager::Instance().Init();
	m_createDataThreadId = CTaskThreadManager::Instance().Init();
}

ProcessWork& ProcessWork::instance()
{
	static ProcessWork ProcessWork;
	return ProcessWork;
}

void ProcessWork::initReceive(ReceiveCallback* callback)
{
	m_callback = callback;
	ReceiveTask* workTask = new ReceiveTask;
	workTask->setClient(this);
	std::shared_ptr<ReceiveTask> spWorkTask;
	spWorkTask.reset(workTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_receiveThreadId)->PostTask(spWorkTask);
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

void ProcessWork::uninit()
{
	CTaskThreadManager::Instance().Uninit(m_receiveThreadId);
	CTaskThreadManager::Instance().Uninit(m_workThreadId);
	CTaskThreadManager::Instance().Uninit(m_deleteThreadId);
}