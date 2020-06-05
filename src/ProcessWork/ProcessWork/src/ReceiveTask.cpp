#include "ReceiveTask.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProcessReceiveCallback.h"

ReceiveTask::ReceiveTask():
m_assign(0),
m_callback(nullptr),
m_memoryMap(nullptr)
{
	
}

void ReceiveTask::DoTask()
{
	auto& memoryPair = m_memoryMap->find(m_assign)->second;
	void* memory = memoryPair.first->writeWithoutLock();
	int32_t sendPid = *(int32_t*)memory;
	int32_t length = *((int32_t*)memory + 1) - 4;
	CorrespondParam::ProtocolId protocolId = (CorrespondParam::ProtocolId)*((int32_t*)memory + 2);
	char* buffer = new char[length + 1];
	buffer[length] = 0;
	::memcpy(buffer, (char*)memory + 12, length);
	*(memoryPair.second) = false;
	m_callback->receive(sendPid, buffer, length, protocolId);
	delete[] buffer;
}

void ReceiveTask::setParam(int32_t assign,
	ProcessReceiveCallback* callback,
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* memoryMap)
{
	m_assign = assign;
	m_callback = callback;
	m_memoryMap = memoryMap;
}