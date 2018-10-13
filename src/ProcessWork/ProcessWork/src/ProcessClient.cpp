#include "ProcessClient.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"

ProcessClient::ProcessClient():
m_memory(nullptr),
m_processMutex(nullptr),
m_event(nullptr)
{
    m_processMutex = new ProcessReadWriteMutex("ProcessWorkMutex");
    m_event = ::CreateEvent(NULL, FALSE, FALSE, "ProcessWorkEvent");
}

void ProcessClient::connect(const std::string& mapName)
{
    m_memory = new SharedMemory(mapName, 1024);
}

void ProcessClient::send(char* buffer, int32_t length, bool isOrdered)
{
    if (m_memory == nullptr || m_processMutex == nullptr)
    {
        return;
    }

    {
        //writememory
        WriteLock writeLock(*m_memory);
        void* memory = m_memory->memory();
        ::memcpy(memory, buffer, length);
    }
    //unlock
    ::SetEvent(m_event);
    //m_processMutex->unwrite();
}