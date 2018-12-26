#include "DeleteDataTask.h"
#include "HandleManager.h"
#include "ProcessWork.h"
#include "SharedMemoryManager.h"

DeleteDataTask::DeleteDataTask() :
m_hCreateData(nullptr)
{

}

void DeleteDataTask::DoTask()
{
	//É¾³ýÔ¿³×ÄÚ´æ
	SharedMemoryManager::instance().deleteData(m_index);
}

void DeleteDataTask::setClient(ProcessWork* client)
{
	m_client = client;
}

void DeleteDataTask::setDeleteDataIndex(int32_t index)
{
	m_index = index;
}