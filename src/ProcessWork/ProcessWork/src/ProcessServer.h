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

/** �����ڴ�ͻ��ˣ��ͻ���ֻ�ܷ���
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
	//��ǰ������ǰȡ����ǰ���ڴ�أ���ǰȡ�ڴ��
	SharedMemory* m_position;
    ProcessReadWriteMutex* m_processMutex;
    uint32_t m_listenThreadId;
	uint32_t m_createThreadId;
    uint32_t m_workThreadId;
	int32_t m_currentPosition;
	LockFreeQueue<SharedMemory*>* m_memoryQueue;
};