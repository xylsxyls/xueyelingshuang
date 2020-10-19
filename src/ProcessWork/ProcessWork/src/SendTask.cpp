#include "SendTask.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "ProcessWork.h"
#include <string.h>

SendTask::SendTask(const char* buffer, int32_t length):
m_thisProcessPid(0),
m_destPid(0),
m_buffer(nullptr),
m_length(0),
m_protocolId(CorrespondParam::ProtocolId::INIT)
{
	m_buffer = new char[length];
	::memcpy(m_buffer, buffer, length);
	m_length = length;
}

SendTask::~SendTask()
{
	delete[] m_buffer;
}

void SendTask::DoTask()
{
	if (m_destPid == 0)
	{
		ProcessWork::instance().send(m_processName, m_buffer, m_length, m_protocolId);
		return;
	}
	ProcessWork::instance().send(m_destPid, m_buffer, m_length, m_protocolId);
}

void SendTask::setParam(int32_t thisProcessId,
	int32_t destPid,
	const std::string& processName,
	CorrespondParam::ProtocolId protocolId)
{
	m_thisProcessPid = thisProcessId;
	m_destPid = destPid;
	m_processName = processName;
	m_protocolId = protocolId;
}