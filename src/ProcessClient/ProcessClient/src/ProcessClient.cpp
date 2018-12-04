#include "ProcessClient.h"
#include "../../../ProcessServer/ProcessServer/ProcessHelper.h"
#include "../../../ProcessServer/ProcessServer/HandleManager.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "ReceiveTask.h"
#include "CSystem/CSystemAPI.h"

ProcessClient::ProcessClient():
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
m_sendPid(0)
{
	m_sendPid = CSystem::processPid();
	m_receiveThreadId = CTaskThreadManager::Instance().Init();
	m_workThreadId = CTaskThreadManager::Instance().Init();
	m_deleteThreadId = CTaskThreadManager::Instance().Init();
	m_positionMutex = new ProcessReadWriteMutex(ProcessHelper::positionMutexName());
	m_position = new SharedMemory(ProcessHelper::positionMapName());
	m_data = new SharedMemory(ProcessHelper::dataMapName(m_position));
	m_key = new SharedMemory(ProcessHelper::keyMapName(m_position));
	m_readData = new SharedMemory(ProcessHelper::dataMapName(m_position));
	m_readKey = new SharedMemory(ProcessHelper::keyMapName(m_position));
}

ProcessClient& ProcessClient::instance()
{
	static ProcessClient processClient;
	return processClient;
}

void ProcessClient::initReceive(ReceiveCallback* callback)
{
	m_callback = callback;
	ReceiveTask* workTask = new ReceiveTask;
	workTask->setClient(this);
	std::shared_ptr<ReceiveTask> spWorkTask;
	spWorkTask.reset(workTask);
	CTaskThreadManager::Instance().GetThreadInterface(m_receiveThreadId)->PostTask(spWorkTask);
}

void ProcessClient::send(char* buffer, int32_t length, int32_t pid, bool isOrdered, int32_t protocolId)
{
	void* data = nullptr;
	int32_t sendIndex = 0;
	int32_t sendBegin = 0;
	{
		WriteLock writeLock(*m_positionMutex);
		//�л�����ǰ
		ProcessHelper::changeToCurrentData(&m_data, m_position);

		//�޸ķ���λ��
		if (!ProcessHelper::addDataPosition(m_position, length))
		{
			//������ɹ��������¹����ڴ�
			::ReleaseSemaphore(HandleManager::instance().createDataHandle(), 1, nullptr);
			::WaitForSingleObject(HandleManager::instance().createDataEndHandle(), INFINITE);
			//�л�����ǰ
			ProcessHelper::changeToCurrentData(&m_data, m_position);
			//�޸ķ���λ��
			if (!ProcessHelper::addDataPosition(m_position, length))
			{
				LOGERROR("change error");
			}
		}
		//�޸ĵ�ǰ�����ڴ���ʹ�ô�С
		ProcessHelper::addDataAlreadyUsed(m_data, length);
		data = m_data->writeWithoutLock();

		sendIndex = ProcessHelper::dataIndex(m_position);
		sendBegin = ProcessHelper::dataPosition(m_position) - length;
	}

	//����
	::memcpy((char*)data + sendBegin, buffer, length);

	{
		WriteLock writeLock(*m_positionMutex);
		//�л�����ǰ
		ProcessHelper::changeToCurrentKey(&m_key, m_position);
		//�޸�Կ��λ��
		if (!ProcessHelper::addKeyPosition(m_position))
		{
			//������ɹ��������¹����ڴ�
			::ReleaseSemaphore(HandleManager::instance().createKeyHandle(), 1, nullptr);
			::WaitForSingleObject(HandleManager::instance().createKeyEndHandle(), INFINITE);
			//�л�����ǰ
			ProcessHelper::changeToCurrentKey(&m_key, m_position);
			//�޸�Կ��λ��
			if (!ProcessHelper::addKeyPosition(m_position))
			{
				LOGERROR("key change error");
			}
		}
		KeyPackage keyPackage;
		keyPackage.m_sendPid = m_sendPid;
		keyPackage.m_receivePid = pid;
		keyPackage.m_length = length;
		keyPackage.m_protocal = protocolId;
		keyPackage.m_index = sendIndex;
		keyPackage.m_begin = sendBegin;
		//д��Կ��
		ProcessHelper::writeKey(m_key, m_position, keyPackage);
	}
	
	//֪ͨ
	::ReleaseSemaphore(HandleManager::instance().assignHandle(), 1, nullptr);
}