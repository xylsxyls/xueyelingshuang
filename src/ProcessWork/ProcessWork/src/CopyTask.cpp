#include "CopyTask.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ReceiveTask.h"

CopyTask::CopyTask() :
m_assign(0),
m_callback(nullptr),
m_memoryMap(nullptr),
m_receiveThread(nullptr)
{
	
}

void CopyTask::DoTask()
{
	auto& memoryPair = m_memoryMap->find(m_assign)->second;
	void* memory = memoryPair.first->writeWithoutLock();
	int32_t sendPid = *(int32_t*)memory;
	int32_t length = *((int32_t*)memory + 1) - 4;
	CorrespondParam::ProtocolId protocolId = (CorrespondParam::ProtocolId)*((int32_t*)memory + 2);
	std::shared_ptr<ReceiveTask> spReceiveTask(new ReceiveTask((char*)memory + 12, length));
	*(memoryPair.second) = false;
	spReceiveTask->setParam(sendPid, m_callback, protocolId);
	m_receiveThread->PostTask(spReceiveTask);
}

void CopyTask::setParam(int32_t assign,
	ProcessReceiveCallback* callback,
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* memoryMap,
	const std::shared_ptr<CTaskThread>& receiveThread)
{
	m_assign = assign;
	m_callback = callback;
	m_memoryMap = memoryMap;
	m_receiveThread = receiveThread;
}