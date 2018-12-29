#include "DeleteKeyTask.h"
#include "HandleManager.h"
#include "ProcessWork.h"
#include "SharedMemoryManager.h"

DeleteKeyTask::DeleteKeyTask() :
m_hCreateData(nullptr)
{

}

void DeleteKeyTask::DoTask()
{
	//删除最前面的钥匙内存
	SharedMemoryManager::instance().deleteKey(m_deleteMemory);
}

void DeleteKeyTask::setClient(ProcessWork* client)
{
	m_client = client;
}

void DeleteKeyTask::setDeleteMemory(SharedMemory* deleteMemory)
{
	m_deleteMemory = deleteMemory;
}