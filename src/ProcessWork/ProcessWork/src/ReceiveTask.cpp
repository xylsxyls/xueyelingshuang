#include "ReceiveTask.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProcessReceiveCallback.h"
#include <string.h>

ReceiveTask::ReceiveTask():
m_sendPid(0),
m_length(0),
m_buffer(nullptr),
m_callback(nullptr),
m_type(MessageType::INIT)
{
	
}

ReceiveTask::~ReceiveTask()
{
	if (m_buffer != nullptr)
	{
		delete[] m_buffer;
	}
}

void ReceiveTask::DoTask()
{
	for (auto itCallback = m_callback->begin(); itCallback != m_callback->end(); ++itCallback)
	{
		(*itCallback)->receive(m_sendPid, m_buffer, m_length, m_type);
	}
	delete[] m_buffer;
	m_buffer = nullptr;
}

void ReceiveTask::setParam(const char* buffer,
	int32_t length,
	int32_t sendPid,
	std::vector<ProcessReceiveCallback*>* callback,
	MessageType type)
{
	m_buffer = new char[length + 1];
	m_buffer[length] = 0;
	::memcpy(m_buffer, buffer, length);
	m_length = length;
	m_sendPid = sendPid;
	m_callback = callback;
	m_type = type;
}