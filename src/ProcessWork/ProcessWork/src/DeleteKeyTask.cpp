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
	//É¾³ýÔ¿³×ÄÚ´æ
	SharedMemoryManager::instance().deleteKey();
}

void DeleteKeyTask::setClient(ProcessWork* client)
{
	m_client = client;
}