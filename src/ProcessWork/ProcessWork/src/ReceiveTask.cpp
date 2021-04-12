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
m_protocolId(CorrespondParam::ProtocolId::INIT)
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
	m_callback->receive(m_sendPid, m_buffer, m_length, m_protocolId);
	delete[] m_buffer;
	m_buffer = nullptr;
}

void ReceiveTask::setParam(const char* buffer,
	int32_t length,
	int32_t sendPid,
	ProcessReceiveCallback* callback,
	CorrespondParam::ProtocolId protocolId)
{
	m_buffer = new char[length + 1];
	m_buffer[length] = 0;
	::memcpy(m_buffer, buffer, length);
	m_length = length;
	m_sendPid = sendPid;
	m_callback = callback;
	m_protocolId = protocolId;
}