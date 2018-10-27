#include "ProcessClient.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"

ProcessClient::ProcessClient():
m_memory(nullptr),
m_processMutex(nullptr),
m_semaphore(nullptr)
{
    m_processMutex = new ProcessReadWriteMutex("ProcessWorkMutex");
	m_semaphore = ::CreateSemaphore(NULL, 0, 0xffff, "ProcessWorkSemaphore");
}

void ProcessClient::connect(const std::string& mapName)
{
    m_memory = new SharedMemory(mapName, 1024);
	m_position = new SharedMemory(mapName + "Position", 16);
}

void ProcessClient::send(char* buffer, int32_t length, bool isOrdered)
{
    if (m_memory == nullptr || m_processMutex == nullptr)
    {
        return;
    }

	int32_t currentPosition = 0;
	{
		//readposition
		ReadLock readLock(*m_position);
		void* position = m_position->memory();
		currentPosition = *((int32_t*)position);
	}
    {
        //writememory
        WriteLock writeLock(*m_memory);
        void* memory = m_memory->memory();
		void* currentMemory = (char*)memory + currentPosition;
		*((int32_t*)currentMemory) = length;
		::memcpy((char*)currentMemory + sizeof(int32_t), buffer, length);
    }
	{
		//writeposition
		WriteLock writeLock(*m_position);
		void* position = m_position->memory();
		*((int32_t*)position) = currentPosition + length + sizeof(int32_t);
	}
	
    //unlock
	::ReleaseSemaphore(m_semaphore, 1, NULL);
    //m_processMutex->unwrite();
}