#pragma once
#include <stdint.h>
#include <list>
#include <map>
#include <mutex>
#include "LockFreeQueue/LockFreeQueueAPI.h"

class SharedMemory;

class ProcessServer
{
	friend class AssignTask;
	friend class CreateDataTask;
	friend class CreateKeyTask;
	friend class DeleteDataTask;
public:
	ProcessServer();

public:
	void listen();

	void uninit();

private:
	SharedMemory* m_position;
	SharedMemory* m_data;
	SharedMemory* m_key;

	uint32_t m_assignThreadId;
	uint32_t m_createDataThreadId;
	uint32_t m_createKeyThreadId;
	uint32_t m_deleteDataThreadId;

	std::map<int32_t, SharedMemory*> m_dataMap;
	std::mutex m_dataMutex;
	LockFreeQueue<SharedMemory*> m_keyList;
};