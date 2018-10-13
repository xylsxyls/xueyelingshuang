#pragma once
#include "ProcessWorkMacro.h"
#include <string>
#include <stdint.h>

class ServerReceiveCallback;
class SharedMemory;
class ProcessReadWriteMutex;

/** �����ڴ�ͻ��ˣ��ͻ���ֻ�ܷ���
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