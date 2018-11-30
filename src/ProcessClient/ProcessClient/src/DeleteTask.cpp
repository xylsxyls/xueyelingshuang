#include "DeleteTask.h"
#include "ProcessClient.h"

DeleteTask::DeleteTask():
m_client(nullptr)
{

}

void DeleteTask::DoTask()
{
	auto itMemory = m_client->m_dataMemoryMap.find(m_index);
	if (itMemory == m_client->m_dataMemoryMap.end())
	{
		return;
	}
	delete itMemory->second;
}

void DeleteTask::setDataMemoryIndex(int32_t index)
{
	m_index = index;
}

