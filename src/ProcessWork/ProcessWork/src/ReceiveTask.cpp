#include "ReceiveTask.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProcessReceiveCallback.h"
#include <string.h>

ReceiveTask::ReceiveTask(const char* buffer, int32_t length):
m_sendPid(0),
m_length(0),
m_buffer(nullptr),
m_callback(nullptr),
m_protocolId(CorrespondParam::ProtocolId::INIT)
{
	m_buffer = new char[length + 1];
	m_buffer[length] = 0;
	::memcpy(m_buffer, buffer, length);
	m_length = length;
}

ReceiveTask::~ReceiveTask()
{
	delete[] m_buffer;
}

void ReceiveTask::DoTask()
{
	m_callback->receive(m_sendPid, m_buffer, m_length, m_protocolId);
}

void ReceiveTask::setParam(int32_t sendPid,
	ProcessReceiveCallback* callback,
	CorrespondParam::ProtocolId protocolId)
{
	m_sendPid = sendPid;
	m_callback = callback;
	m_protocolId = protocolId;
}