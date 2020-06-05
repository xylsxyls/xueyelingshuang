#include "ProcessWork.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "AssignTask.h"
#include "ReadTask.h"

ProcessWork::ProcessWork():
m_thisProcessPid(0),
m_callback(nullptr),
m_assignSemaphore(nullptr),
m_assignEndSemaphore(nullptr),
m_readSemaphore(nullptr),
m_readEndSemaphore(nullptr),
m_area(nullptr),
m_assginThreadId(0),
m_readThreadId(0),
m_receiveThreadId(0)
{
	m_thisProcessPid = CSystem::processFirstPid();
}

ProcessWork& ProcessWork::instance()
{
	static ProcessWork ProcessWork;
	return ProcessWork;
}

void ProcessWork::initReceive(ProcessReceiveCallback* callback, int32_t areaCount, int32_t receiveSize)
{
	m_callback = callback;

	m_area = new SharedMemory(CStringManager::Format("ProcessArea_%d", m_thisProcessPid), 8);

	m_assignSemaphore = new Semaphore;
	m_assignEndSemaphore = new Semaphore;
	m_readSemaphore = new Semaphore;
	m_readEndSemaphore = new Semaphore;

	m_assignSemaphore->createProcessSemaphore(CStringManager::Format("ProcessAssgin_%d", m_thisProcessPid));
	m_assignEndSemaphore->createProcessSemaphore(CStringManager::Format("ProcessAssginEnd_%d", m_thisProcessPid));
	m_readSemaphore->createProcessSemaphore(CStringManager::Format("ProcessRead_%d", m_thisProcessPid));
	m_readEndSemaphore->createProcessSemaphore(CStringManager::Format("ProcessReadEnd_%d", m_thisProcessPid));

	int32_t index = -1;
	while (index++ != areaCount - 1)
	{
		std::shared_ptr<SharedMemory> spSharedMemory(new SharedMemory(CStringManager::Format("ProcessArea_%d_%d", m_thisProcessPid, index + 1), receiveSize));
		m_memoryMap[index + 1].first = spSharedMemory;
		std::shared_ptr<std::atomic<bool>> spUsed(new std::atomic<bool>(false));
		m_memoryMap[index + 1].second = spUsed;
	}

	m_assginThreadId = CTaskThreadManager::Instance().Init();
	m_readThreadId = CTaskThreadManager::Instance().Init();
	m_receiveThreadId = CTaskThreadManager::Instance().Init();

	std::shared_ptr<AssignTask> spAssignTask(new AssignTask);
	spAssignTask->setParam(m_assignSemaphore, m_assignEndSemaphore, m_area, &m_memoryMap);
	CTaskThreadManager::Instance().GetThreadInterface(m_assginThreadId)->PostTask(spAssignTask);

	std::shared_ptr<ReadTask> spReadTask(new ReadTask);
	spReadTask->setParam(m_callback, m_readSemaphore, m_readEndSemaphore, m_area, &m_memoryMap, CTaskThreadManager::Instance().GetThreadInterface(m_receiveThreadId));
	CTaskThreadManager::Instance().GetThreadInterface(m_readThreadId)->PostTask(spReadTask);
}

void ProcessWork::uninitReceive()
{
	CTaskThreadManager::Instance().Uninit(m_assginThreadId);
	CTaskThreadManager::Instance().Uninit(m_readThreadId);
	CTaskThreadManager::Instance().Uninit(m_receiveThreadId);
	m_memoryMap.clear();
	m_assignSemaphore->closeProcessSemaphore();
	m_assignEndSemaphore->closeProcessSemaphore();
	m_readSemaphore->closeProcessSemaphore();
	m_readEndSemaphore->closeProcessSemaphore();
	delete m_assignSemaphore;
	delete m_assignEndSemaphore;
	delete m_readSemaphore;
	delete m_readEndSemaphore;
	delete m_area;

	m_assignSemaphore = nullptr;
	m_assignEndSemaphore = nullptr;
	m_readSemaphore = nullptr;
	m_readEndSemaphore = nullptr;
	m_area = nullptr;

	m_assginThreadId = 0;
	m_readThreadId = 0;
	m_receiveThreadId = 0;
	m_callback = nullptr;
}

void ProcessWork::send(int32_t destPid, const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	if (destPid == 0)
	{
		return;
	}

	int32_t assign = 0;
	SharedMemory destArea(CStringManager::Format("ProcessArea_%d", destPid));
	void* area = destArea.writeWithoutLock();

	ProcessReadWriteMutex destProcessAssignMutex(CStringManager::Format("ProcessAssginMutex_%d", destPid));
	ProcessReadWriteMutex destProcessReadMutex(CStringManager::Format("ProcessReadMutex_%d", destPid));

	Semaphore destAssignSemaphore;
	Semaphore destAssignEndSemaphore;
	Semaphore destReadSemaphore;
	Semaphore destReadEndSemaphore;

	destAssignSemaphore.openProcessSemaphore(CStringManager::Format("ProcessAssgin_%d", destPid));
	destAssignEndSemaphore.openProcessSemaphore(CStringManager::Format("ProcessAssginEnd_%d", destPid));
	destReadSemaphore.openProcessSemaphore(CStringManager::Format("ProcessRead_%d", destPid));
	destReadEndSemaphore.openProcessSemaphore(CStringManager::Format("ProcessReadEnd_%d", destPid));

	//申请缓存区号
	{
		WriteLock writeLock(destProcessAssignMutex);
		destAssignSemaphore.processSignal();
		destAssignEndSemaphore.processWait();
		//读取申请的缓存区号
		assign = (int32_t)(*((int32_t*)area));
	}
	SharedMemory sharedMemory(CStringManager::Format("ProcessArea_%d_%d", destPid, assign));
	void* memory = sharedMemory.writeWithoutLock();
	*((int32_t*)memory) = m_thisProcessPid;
	*((int32_t*)memory + 1) = length + 4;
	*((int32_t*)memory + 2) = (int32_t)protocolId;
	::memcpy((char*)memory + 12, buffer, length);
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

void ProcessWork::send(const std::string& processName, const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	std::vector<int32_t> vecPid = CSystem::processPid(processName);
	if (vecPid.empty())
	{
		return;
	}
	send(vecPid[0], buffer, length, protocolId);
}

//class Receive : public ProcessReceiveCallback
//{
//public:
//	void receive(int32_t sendPid, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
//	{
//		RCSend("%s", buffer);
//	}
//};
//
//int32_t main()
//{
//	Receive receive;
//	ProcessWork::instance().initReceive(&receive, 20000);
//	getchar();
//	return 0;
//}