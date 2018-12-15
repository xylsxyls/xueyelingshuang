#include "WorkTask.h"
#include "../../../ProcessServer/ProcessServer/ProcessHelper.h"
#include "../../../ProcessServer/ProcessServer/HandleManager.h"
#include "ProcessWork.h"
#include "ReceiveCallback.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "DeleteTask.h"

WorkTask::WorkTask():
m_client(nullptr),
m_exit(false)
{

}

void WorkTask::DoTask()
{
	char* buffer = new char[m_keyPackage.m_length + 1];
	buffer[m_keyPackage.m_length] = 0;
	ProcessHelper::readData(buffer, m_keyPackage, m_client->m_readData);
	m_client->m_callback->receive(buffer, m_keyPackage.m_length, m_keyPackage.m_sendPid, m_keyPackage.m_protocal);
	delete[] buffer;
	{
		//修改当前内存大小
		WriteLock writeLock(*m_client->m_positionMutex);
		//减少当前内存使用大小
		ProcessHelper::reduceDataAlreadyUsed(m_client->m_readData, m_keyPackage.m_length);
		//判断是否需要删除共享内存
		if (ProcessHelper::needDeleteDataMemory(m_client->m_readData, m_client->m_position))
		{
			ProcessHelper::writeDeleteDataIndex(m_client->m_position, m_keyPackage.m_index);
			::ReleaseSemaphore(HandleManager::instance().deleteDataHandle(), 1, nullptr);
			m_hDeleteEnd = HandleManager::instance().deleteDataEndHandle();
			::WaitForSingleObject(m_hDeleteEnd, INFINITE);
			if (m_exit)
			{
				return;
			}
		}
	}
}

void WorkTask::StopTask()
{
	m_exit = true;
	::ReleaseSemaphore(m_hDeleteEnd, 1, nullptr);
}

void WorkTask::setKey(const KeyPackage& keyPackage)
{
	m_keyPackage = keyPackage;
}

void WorkTask::setClient(ProcessWork* client)
{
	m_client = client;
}
