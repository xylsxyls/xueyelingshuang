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
	SharedMemoryManager::instance().deleteKey();
}

void DeleteKeyTask::setClient(ProcessWork* client)
{
	m_client = client;
}