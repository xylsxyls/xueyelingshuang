#include "SendTask.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include "Semaphore/SemaphoreAPI.h"

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
		SharedMemory destArea(CStringManager::Format("ProcessArea_%s", m_processName.c_str()));
		void* pid = destArea.writeWithoutLock();
		if (pid == nullptr)
		{
			return;
		}
		m_destPid = *(int32_t*)pid;
	}
	
	int32_t assign = 0;
	SharedMemory destArea(CStringManager::Format("ProcessArea_%d", m_destPid));
	void* area = destArea.writeWithoutLock();

	ProcessReadWriteMutex destProcessAssignMutex(CStringManager::Format("ProcessAssginMutex_%d", m_destPid));
	ProcessReadWriteMutex destProcessReadMutex(CStringManager::Format("ProcessReadMutex_%d", m_destPid));

	Semaphore destAssignSemaphore;
	Semaphore destAssignEndSemaphore;
	Semaphore destReadSemaphore;
	Semaphore destReadEndSemaphore;

	destAssignSemaphore.openProcessSemaphore(CStringManager::Format("ProcessAssgin_%d", m_destPid));
	destAssignEndSemaphore.openProcessSemaphore(CStringManager::Format("ProcessAssginEnd_%d", m_destPid));
	destReadSemaphore.openProcessSemaphore(CStringManager::Format("ProcessRead_%d", m_destPid));
	destReadEndSemaphore.openProcessSemaphore(CStringManager::Format("ProcessReadEnd_%d", m_destPid));

	//申请缓存区号
	{
		WriteLock writeLock(destProcessAssignMutex);
		destAssignSemaphore.processSignal();
		destAssignEndSemaphore.processWait();
		//读取申请的缓存区号
		assign = (int32_t)(*((int32_t*)area));
	}
	SharedMemory sharedMemory(CStringManager::Format("ProcessArea_%d_%d", m_destPid, assign));
	void* memory = sharedMemory.writeWithoutLock();
	*((int32_t*)memory) = m_thisProcessPid;
	*((int32_t*)memory + 1) = m_length + 4;
	*((int32_t*)memory + 2) = (int32_t)m_protocolId;
	::memcpy((char*)memory + 12, m_buffer, m_length);
	//申请目标读取
	{
		WriteLock writeLock(destProcessReadMutex);
		destReadEndSemaphore.processWait();
		//把写好的缓存区号写进去
		*((int32_t*)area + 1) = assign;
		destReadSemaphore.processSignal();
	}

	destAssignSemaphore.closeProcessSemaphore();
	destAssignEndSemaphore.closeProcessSemaphore();
	destReadSemaphore.closeProcessSemaphore();
	destReadEndSemaphore.closeProcessSemaphore();
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