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
	void* memory = m_memoryMap->find(m_assign)->second->writeWithoutLock();
	if (memory == nullptr)
	{
		printf("CopyTask memory nullptr\n");
	}
	void* areaAssign = m_areaAssign->writeWithoutLock();
	if (areaAssign == nullptr)
	{
		printf("CopyTask areaAssign nullptr\n");
	}
	int32_t areaCount = *(int32_t*)areaAssign;
	int32_t sendPid = *(int32_t*)memory;
	int32_t length = *((int32_t*)memory + 1) - 4;
	MessageType type = (MessageType)*((int32_t*)memory + 2);
	std::shared_ptr<ReceiveTask> spReceiveTask(new ReceiveTask);
	spReceiveTask->setParam((char*)memory + 12, length, sendPid, m_callback, type);

	int32_t index = -1;
	while (index++ != areaCount - 1)
	{
		if (*((int32_t*)areaAssign + 1 + index * 3) == m_assign)
		{
			*((int32_t*)areaAssign + 1 + index * 3 + 2) = 0;
			*((int32_t*)areaAssign + 1 + index * 3 + 1) = 0;
			break;
		}
	}

	m_receiveThread->PostTask(spReceiveTask);
}

void CopyTask::setParam(int32_t assign,
	std::vector<ProcessReceiveCallback*>* callback,
	SharedMemory* areaAssign,
	std::map<int32_t, std::shared_ptr<SharedMemory>>* memoryMap,
	const std::shared_ptr<CTaskThread>& receiveThread)
{
	m_assign = assign;
	m_callback = callback;
	m_areaAssign = areaAssign;
	m_memoryMap = memoryMap;
	m_receiveThread = receiveThread;
}