#include "ProcessWork.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "AssignTask.h"
#include "ReadTask.h"
#include "SendTask.h"

ProcessWork::ProcessWork():
m_thisProcessPid(0),
m_callback(nullptr),
m_assignSemaphore(nullptr),
m_assignEndSemaphore(nullptr),
m_readSemaphore(nullptr),
m_readEndSemaphore(nullptr),
m_area(nullptr),
m_pid(nullptr),
m_assginThreadId(0),
m_readThreadId(0),
m_copyThreadId(0),
m_receiveThreadId(0),
m_sendThreadId(0)
{
	m_thisProcessPid = CSystem::processFirstPid();
	m_sendThreadId = CTaskThreadManager::Instance().Init();
}

ProcessWork& ProcessWork::instance()
{
	static ProcessWork ProcessWork;
	return ProcessWork;
}

void ProcessWork::initReceive(ProcessReceiveCallback* callback, int32_t receiveSize, int32_t areaCount)
{
	m_callback = callback;

	m_area = new SharedMemory(CStringManager::Format("ProcessArea_%d", m_thisProcessPid), 8);

	m_pid = new SharedMemory(CStringManager::Format("ProcessArea_%s", CSystem::GetCurrentExeName().c_str()), 4);
	void* pid = m_pid->writeWithoutLock();
	*(int32_t*)pid = m_thisProcessPid;

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
	m_copyThreadId = CTaskThreadManager::Instance().Init();
	m_receiveThreadId = CTaskThreadManager::Instance().Init();

	std::shared_ptr<AssignTask> spAssignTask(new AssignTask);
	spAssignTask->setParam(m_assignSemaphore, m_assignEndSemaphore, m_area, &m_memoryMap);
	CTaskThreadManager::Instance().GetThreadInterface(m_assginThreadId)->PostTask(spAssignTask);

	std::shared_ptr<ReadTask> spReadTask(new ReadTask);
	spReadTask->setParam(m_callback,
		m_readSemaphore,
		m_readEndSemaphore,
		m_area,
		&m_memoryMap,
		CTaskThreadManager::Instance().GetThreadInterface(m_copyThreadId),
		CTaskThreadManager::Instance().GetThreadInterface(m_receiveThreadId));
	CTaskThreadManager::Instance().GetThreadInterface(m_readThreadId)->PostTask(spReadTask);
}

void ProcessWork::uninitReceive()
{
	CTaskThreadManager::Instance().Uninit(m_assginThreadId);
	CTaskThreadManager::Instance().Uninit(m_readThreadId);
	CTaskThreadManager::Instance().Uninit(m_copyThreadId);
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
	delete m_pid;

	m_assignSemaphore = nullptr;
	m_assignEndSemaphore = nullptr;
	m_readSemaphore = nullptr;
	m_readEndSemaphore = nullptr;
	m_area = nullptr;

	m_assginThreadId = 0;
	m_readThreadId = 0;
	m_copyThreadId = 0;
	m_receiveThreadId = 0;
	m_callback = nullptr;
}

void ProcessWork::uninit()
{
	CTaskThreadManager::Instance().Uninit(m_sendThreadId);
	m_sendThreadId = 0;
}

void ProcessWork::send(int32_t destPid, const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	if (destPid <= 0 || (buffer == nullptr && length != 0))
	{
		return;
	}
	std::shared_ptr<SendTask> spSendTask(new SendTask(buffer, length));
	spSendTask->setParam(m_thisProcessPid, destPid, "", protocolId);
	std::shared_ptr<CTaskThread> sendThread = CTaskThreadManager::Instance().GetThreadInterface(m_sendThreadId);
	if (sendThread == nullptr)
	{
		return;
	}
	sendThread->PostTask(spSendTask);
}

void ProcessWork::send(const std::string& processName, const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	if (buffer == nullptr && length != 0)
	{
		return;
	}
	std::shared_ptr<SendTask> spSendTask(new SendTask(buffer, length));
	spSendTask->setParam(m_thisProcessPid, 0, processName, protocolId);
	std::shared_ptr<CTaskThread> sendThread = CTaskThreadManager::Instance().GetThreadInterface(m_sendThreadId);
	if (sendThread == nullptr)
	{
		return;
	}
	sendThread->PostTask(spSendTask);
}

//#include "ProcessWork/ProcessWorkAPI.h"
//#include "CSystem/CSystemAPI.h"
//#include "CStringManager/CStringManagerAPI.h"
//
//int32_t num = 0;
//
//class Receive : public ProcessReceiveCallback
//{
//public:
//	void receive(int32_t sendPid, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
//	{
//		RCSend("num = %d, %s", num, buffer);
//		ProcessWork::instance().send("ConsoleTest" + std::to_string(num + 1), "123456", 6);
//	}
//};
//
//int32_t main()
//{
//	std::string currentName = CSystem::GetCurrentExeName();
//	CStringManager::Replace(currentName, "ConsoleTest", "");
//	num = atoi(currentName.c_str());
//	std::string name = "ConsoleTest" + std::to_string(num + 1);
//
//	Receive receive;
//	ProcessWork::instance().initReceive(&receive, 1024, 1000);
//	Sleep(10000);
//
//	std::string str;
//	int32_t strCount = 1;
//	while (strCount-- != 0)
//	{
//		str += "123456";
//	}
//
//	int32_t count = 10000;
//	while (count-- != 0)
//	{
//		ProcessWork::instance().send(name.c_str(), str.c_str(), str.length());
//		ProcessWork::instance().send("ConsoleTest10", str.c_str(), str.length());
//	}
//	getchar();
//	return 0;
//}
//
//copy ConsoleTest.exe ConsoleTest1.exe
//copy ConsoleTest.exe ConsoleTest2.exe
//copy ConsoleTest.exe ConsoleTest3.exe
//copy ConsoleTest.exe ConsoleTest4.exe
//copy ConsoleTest.exe ConsoleTest5.exe
//copy ConsoleTest.exe ConsoleTest6.exe
//copy ConsoleTest.exe ConsoleTest7.exe
//copy ConsoleTest.exe ConsoleTest8.exe
//copy ConsoleTest.exe ConsoleTest9.exe
//copy ConsoleTest.exe ConsoleTest10.exe
//
//start ConsoleTest10.exe
//start ConsoleTest9.exe
//start ConsoleTest8.exe
//start ConsoleTest7.exe
//start ConsoleTest6.exe
//start ConsoleTest5.exe
//start ConsoleTest4.exe
//start ConsoleTest3.exe
//start ConsoleTest2.exe
//start ConsoleTest1.exe