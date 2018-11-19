#include "ProcessClient.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include "WorkParam.h"
#include "CStringManager/CStringManagerAPI.h"
#include "ProcessHelper.h"
#include "D:\\SendToMessageTest.h"
#include "LogManager/LogManagerAPI.h"

ProcessClient::ProcessClient():
m_memory(nullptr),
m_processMutex(nullptr),
m_memorySemaphore(nullptr),
m_positionBeginSemaphore(nullptr),
m_positionEndSemaphore(nullptr)
{
    m_processMutex = new ProcessReadWriteMutex("ProcessWorkMutex");
	m_memorySemaphore = ::CreateSemaphore(NULL, 0, 128000001, "ProcessWorkMemorySemaphore");
	m_positionBeginSemaphore = ::CreateSemaphore(NULL, 0, 128000001, "ProcessWorkPositionBeginSemaphore");
	m_positionEndSemaphore = ::CreateSemaphore(NULL, 0, 128000001, "ProcessWorkPositionEndSemaphore");
}

bool ProcessClient::connect(const std::string& mapName)
{
	m_mapName = mapName;
	m_position = new SharedMemory(ProcessHelper::positionMapName(mapName));
	WriteLock writeLock(*m_position);
	void* position = m_position->memory();
	if (position == nullptr)
	{
		return false;
	}
	m_memory = new SharedMemory(ProcessHelper::sendMemoryMapName(mapName, position));
	return true;
}

void ProcessClient::send(char* buffer, int32_t length, bool isOrdered, int32_t protocolId)
{
    if (m_memory == nullptr || m_processMutex == nullptr)
    {
        return;
    }

	static int sendCount = 0;

	if (sendCount == 0)
	{
		//LogManager::instance().init("D:\\xueyelingshuang\\lib\\ProcessServerd.log");
	}

	int32_t sendPosition = 0;
	{
		//readposition
		WriteLock writeLock(*m_position);
		void* position = m_position->memory();

		//LOGINFO("changeToCurrentSendMemory");
		ProcessHelper::changeToCurrentSendMemory(&m_memory, m_mapName, position);

		sendPosition = ProcessHelper::sendPosition(position);
		//writenewposition
		//LOGINFO("addSendPosition");
		if (!ProcessHelper::addSendPosition(position, length + sizeof(int32_t) * 2))
		{
			//writememory
			void* memory = m_memory->writeWithoutLock();
			//LOGINFO("writeEndMemory");
			ProcessHelper::writeEndMemory(memory, sendPosition);

			//LOGINFO("sendCount = %d, buffer = %s", sendCount, buffer);

			::ReleaseSemaphore(m_positionBeginSemaphore, 1, NULL);
			//LOGINFO("release end");
			::WaitForSingleObject(m_positionEndSemaphore, INFINITE);
			//LOGINFO("wait end");
			delete m_memory;
			std::string sendMemoryMapName = ProcessHelper::sendMemoryMapName(m_mapName, position);
			//LOGINFO("new sendMemoryMapName = %s", sendMemoryMapName.c_str());
			m_memory = new SharedMemory(sendMemoryMapName);
			ProcessHelper::resetSendPosition(position);
			sendPosition = 0;
			ProcessHelper::addSendPosition(position, length + sizeof(int32_t) * 2);
			//LOGINFO("apply end");
		}
	}

	//writememory
	//LOGINFO("write memory, mapName = %s, buffer = %s, sendPosition = %d", m_memory->mapName().c_str(), buffer, sendPosition);
	void* memory = m_memory->writeWithoutLock();
	ProcessHelper::writeMemory(memory, sendPosition, buffer, length, protocolId);
	++sendCount;
    //unlock
	//LOGINFO("ReleaseSemaphore");
	//if (sendCount % 10000 == 0)
	//{
	//	RCSend("sendCount = %d", sendCount);
	//}
	::ReleaseSemaphore(m_memorySemaphore, 1, NULL);
    //m_processMutex->unwrite();
}