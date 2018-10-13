#pragma once
#include "ProcessWorkMacro.h"
#include <string>
#include <stdint.h>

class ServerReceiveCallback;
class SharedMemory;
class ProcessReadWriteMutex;

/** 共享内存客户端，客户端只能发送
*/
class ProcessWorkAPI ProcessServer
{
    friend class ListenTask;
    friend class WorkTask;
public:
	ProcessServer();

public:
	void listen(const std::string& mapName, ServerReceiveCallback* receive);

protected:
	ServerReceiveCallback* m_callback;
    SharedMemory* m_memory;
    ProcessReadWriteMutex* m_processMutex;
    uint32_t m_listenThreadId;
    uint32_t m_workThreadId;
};