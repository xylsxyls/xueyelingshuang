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
		
		//修改发送位置
		if (!SharedMemoryManager::instance().addDataPosition(pid, length))
		{
			//如果不成功就申请新共享内存
			::ReleaseSemaphore(HandleManager::instance().createDataHandle(pid, true), 1, nullptr);
			::WaitForSingleObject(HandleManager::instance().createDataEndHandle(pid, true), INFINITE);
			//修改发送位置
			if (!SharedMemoryManager::instance().addDataPosition(pid, length))
			{
				LOGERROR("change error");
			}
		}

		//获取发送数据内存，最新
		SharedMemory* sendData = SharedMemoryManager::instance().getCurrentSendData(pid);

		//修改当前共享内存已使用大小
		SharedMemoryManager::instance().addDataAlreadyUsed(pid, length);
		data = sendData->writeWithoutLock();

		sendIndex = SharedMemoryManager::instance().sendDataIndex(pid);
		sendBegin = SharedMemoryManager::instance().sendDataPosition(pid) - length;
	}

	//拷贝
	::memcpy((char*)data + sendBegin, buffer, length);

	{
		WriteLock writeLock(*(ProcessMutexManager::instance().positionMutex(pid)));
		
		//修改钥匙位置
		if (!SharedMemoryManager::instance().addKeyPosition(pid))
		{
			//如果不成功就申请新共享内存
			::ReleaseSemaphore(HandleManager::instance().createKeyHandle(pid, true), 1, nullptr);
			::WaitForSingleObject(HandleManager::instance().createKeyEndHandle(pid, true), INFINITE);
			//切换至当前
			//ProcessHelper::changeToCurrentKey(&m_key, m_position);
			//修改钥匙位置
			if (!SharedMemoryManager::instance().addKeyPosition(pid))
			{
				LOGERROR("key change error");
			}
		}

		//切换至当前
		//SharedMemoryManager::instance().changeToCurrentKey(pid);

		KeyPackage keyPackage;
		keyPackage.m_sendPid = m_processPid;
		keyPackage.m_receivePid = pid;
		keyPackage.m_length = length;
		keyPackage.m_protocal = protocolId;
		keyPackage.m_index = sendIndex;
		keyPackage.m_begin = sendBegin;
		//先切换到当前钥匙内存再写入钥匙
		SharedMemoryManager::instance().writeKey(pid, keyPackage);
	}
	
	//通知
	::ReleaseSemaphore(HandleManager::instance().assignHandle(pid, true), 1, nullptr);
}

void ProcessWork::send(const char* buffer, int32_t length, const std::string processName, int32_t protocolId)
{
	send(buffer, length, CSystem::processPid(processName), protocolId);
}