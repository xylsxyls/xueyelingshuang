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
	//ɾ����ǰ���Կ���ڴ�
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