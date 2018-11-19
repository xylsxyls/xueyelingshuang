#pragma once
#include "ProcessWorkMacro.h"
#include <string>
#include <stdint.h>
#include <memory>

class ServerReceiveCallback;
class SharedMemory;
class ProcessReadWriteMutex;
template <class SharedMemory>
class LockFreeQueue;

/** 共享内存客户端，客户端只能发送
*/
class ProcessWorkAPI ProcessServer
{
    friend class ListenTask;
    friend class WorkTask;
	friend class CreateMemoryTask;
public:
	ProcessServer();

public:
	void listen(const std::string& mapName, ServerReceiveCallback* receive);

protected:
	ServerReceiveCallback* m_callback;
    SharedMemory* m_memory;
	//当前发，当前取，当前发内存池，当前取内存池
	SharedMemory* m_position;
    ProcessReadWriteMutex* m_processMutex;
    uint32_t m_listenThreadId;
	uint32_t m_createThreadId;
    uint32_t m_workThreadId;
	int32_t m_currentPosition;
	LockFreeQueue<SharedMemory*>* m_memoryQueue;
};