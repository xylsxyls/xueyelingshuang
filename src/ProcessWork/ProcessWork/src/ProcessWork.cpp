#include "ProcessWork.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "AssignTask.h"
#include "ReadTask.h"
#include "SendTask.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include <string.h>
#ifdef __unix__
#include <unistd.h>
#endif

ProcessWork::ProcessWork():
m_thisProcessPid(0),
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
m_postThreadId(0),
m_flow(0)
{
	m_thisProcessPid = CSystem::currentProcessPid();
}

ProcessWork& ProcessWork::instance()
{
	static ProcessWork ProcessWork;
	return ProcessWork;
}

void ProcessWork::addProcessReceiveCallback(ProcessReceiveCallback* callback)
{
	m_callback.push_back(callback);
}

bool ProcessWork::initReceive(int32_t receiveSize, int32_t areaCount, int32_t flow)
{
	m_flow = flow;
	m_sendProcessDeque.setFinite(m_flow);
	m_sendMemoryDeque.setFinite(m_flow * m_flow);
	m_sendMutexDeque.setFinite(m_flow);
	m_sendSemaphoreDeque.setFinite(m_flow);

	m_area = new SharedMemory(CStringManager::Format("ProcessArea_%d", m_thisProcessPid), 8, false);
	if (m_area->isFailed())
	{
		delete m_area;
		m_area = nullptr;
		return false;
	}

#ifdef _MSC_VER
	std::string currentExeName = CSystem::GetCurrentExeName();
#elif __unix__
	std::string currentExeName = CSystem::GetCurrentExeFullName();
#endif
	m_pid = new SharedMemory(CStringManager::Format("ProcessArea_%s", currentExeName.c_str()), 4, false);
	if (m_area->isFailed())
	{
		delete m_area;
		m_area = nullptr;
		delete m_pid;
		m_pid = nullptr;
		return false;
	}
	void* pid = m_pid->writeWithoutLock();
	if (pid == nullptr)
	{
		printf("init pid nullptr\n");
	}

	m_assignSemaphore = new Semaphore;
	m_assignEndSemaphore = new Semaphore;
	m_readSemaphore = new Semaphore;
	m_readEndSemaphore = new Semaphore;

	m_assignSemaphore->createProcessSemaphore(CStringManager::Format("ProcessAssgin_%d", m_thisProcessPid));
	m_assignEndSemaphore->createProcessSemaphore(CStringManager::Format("ProcessAssginEnd_%d", m_thisProcessPid));
	m_readSemaphore->createProcessSemaphore(CStringManager::Format("ProcessRead_%d", m_thisProcessPid));
	m_readEndSemaphore->createProcessSemaphore(CStringManager::Format("ProcessReadEnd_%d", m_thisProcessPid));

	int32_t memoryIndex = 0;
	int32_t index = -1;
	while (index++ != areaCount - 1)
	{
		std::shared_ptr<SharedMemory> spSharedMemory;
		do
		{
			spSharedMemory.reset(new SharedMemory(CStringManager::Format("ProcessArea_%d_%d", m_thisProcessPid, ++memoryIndex), receiveSize, false));
		} while (spSharedMemory->isFailed());
		m_memoryMap[memoryIndex].first = spSharedMemory;
		std::shared_ptr<std::atomic<bool>> spUsed(new std::atomic<bool>(false));
		m_memoryMap[memoryIndex].second = spUsed;
	}

	m_assginThreadId = CTaskThreadManager::Instance().Init();
	m_readThreadId = CTaskThreadManager::Instance().Init();
	m_copyThreadId = CTaskThreadManager::Instance().Init();
	m_receiveThreadId = CTaskThreadManager::Instance().Init();

	std::shared_ptr<AssignTask> spAssignTask(new AssignTask);
	spAssignTask->setParam(m_assignSemaphore, m_assignEndSemaphore, m_area, &m_memoryMap);
	CTaskThreadManager::Instance().GetThreadInterface(m_assginThreadId)->PostTask(spAssignTask);

	std::shared_ptr<ReadTask> spReadTask(new ReadTask);
	spReadTask->setParam(&m_callback,
		m_readSemaphore,
		m_readEndSemaphore,
		m_area,
		&m_memoryMap,
		CTaskThreadManager::Instance().GetThreadInterface(m_copyThreadId),
		CTaskThreadManager::Instance().GetThreadInterface(m_receiveThreadId));
	CTaskThreadManager::Instance().GetThreadInterface(m_readThreadId)->PostTask(spReadTask);

	*(int32_t*)pid = m_thisProcessPid;
	
	return true;
}

void ProcessWork::uninitReceive()
{
	delete m_pid;
	m_pid = nullptr;

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

	m_assignSemaphore = nullptr;
	m_assignEndSemaphore = nullptr;
	m_readSemaphore = nullptr;
	m_readEndSemaphore = nullptr;
	m_area = nullptr;

	m_assginThreadId = 0;
	m_readThreadId = 0;
	m_copyThreadId = 0;
	m_receiveThreadId = 0;
	m_callback.clear();
}

void ProcessWork::clear()
{
	{
		std::unique_lock<std::mutex> lock(m_sendProcessDequeMutex);
		m_sendProcessDeque.clear();
	}
	{
		std::unique_lock<std::mutex> lock(m_sendMemoryDequeMutex);
		m_sendMemoryDeque.clear();
	}
	{
		std::unique_lock<std::mutex> lock(m_sendMutexDequeMutex);
		m_sendMutexDeque.clear();
	}
	{
		std::unique_lock<std::mutex> lock(m_sendSemaphoreDequeMutex);
		m_sendSemaphoreDeque.clear();
	}
}

void ProcessWork::initPostThread()
{
	if (m_postThreadId != 0)
	{
		return;
	}
	m_postThreadId = CTaskThreadManager::Instance().Init();
}

void ProcessWork::uninitPostThread()
{
	if (m_postThreadId == 0)
	{
		return;
	}
	CTaskThreadManager::Instance().Uninit(m_postThreadId);
	m_postThreadId = 0;
}

//可以将申请的缓冲区号改为先分配号一组让发送者有序拿，然后再把写好的分配号放入申请读取组中增加效率，读取端将信号量改为事件，拷贝出分配号一次性读取
//效率可以优化到一秒25万左右
void ProcessWork::send(int32_t destPid, const char* buffer, int32_t length, MessageType type)
{
	if (destPid <= 0)
	{
		return;
	}

	int32_t assign = 0;
	std::shared_ptr<SharedMemory> destArea;
	{
		std::unique_lock<std::mutex> lock(m_sendProcessDequeMutex);
		for (auto it = m_sendProcessDeque.begin(); it != m_sendProcessDeque.end(); ++it)
		{
			if (it->first == destPid)
			{
				destArea = it->second;
				break;
			}
		}
	}
	if (destArea == nullptr)
	{
		destArea.reset(new SharedMemory(CStringManager::Format("ProcessArea_%d", destPid)));
		std::unique_lock<std::mutex> lock(m_sendProcessDequeMutex);
		m_sendProcessDeque.push_back(std::pair<int32_t, std::shared_ptr<SharedMemory>>(destPid, destArea));
	}

	void* area = destArea->writeWithoutLock();
	if (area == nullptr)
	{
		return;
	}

	std::shared_ptr<ProcessReadWriteMutex> destProcessAssignMutex;
	std::shared_ptr<ProcessReadWriteMutex> destProcessReadMutex;
	{
		std::unique_lock<std::mutex> lock(m_sendMutexDequeMutex);
		for (auto it = m_sendMutexDeque.begin(); it != m_sendMutexDeque.end(); ++it)
		{
			if (it->first == destPid)
			{
				destProcessAssignMutex = it->second[0];
				destProcessReadMutex = it->second[1];
				break;
			}
		}
	}
	
	if (destProcessAssignMutex == nullptr || destProcessReadMutex == nullptr)
	{
		destProcessAssignMutex.reset(new ProcessReadWriteMutex(CStringManager::Format("ProcessAssginMutex_%d", destPid)));
		destProcessReadMutex.reset(new ProcessReadWriteMutex(CStringManager::Format("ProcessReadMutex_%d", destPid)));
		std::pair<int32_t, std::shared_ptr<ProcessReadWriteMutex>[2]> pair;
		pair.first = destPid;
		pair.second[0] = destProcessAssignMutex;
		pair.second[1] = destProcessReadMutex;
		std::unique_lock<std::mutex> lock(m_sendMutexDequeMutex);
		m_sendMutexDeque.push_back(pair);
	}

	std::shared_ptr<Semaphore> destAssignSemaphore;
	std::shared_ptr<Semaphore> destAssignEndSemaphore;
	std::shared_ptr<Semaphore> destReadSemaphore;
	std::shared_ptr<Semaphore> destReadEndSemaphore;
	{
		std::unique_lock<std::mutex> lock(m_sendSemaphoreDequeMutex);
		for (auto it = m_sendSemaphoreDeque.begin(); it != m_sendSemaphoreDeque.end(); ++it)
		{
			if (it->first == destPid)
			{
				destAssignSemaphore = it->second[0];
				destAssignEndSemaphore = it->second[1];
				destReadSemaphore = it->second[2];
				destReadEndSemaphore = it->second[3];
				break;
			}
		}
	}
	
	if (destAssignSemaphore == nullptr ||
		destAssignEndSemaphore == nullptr ||
		destReadSemaphore == nullptr ||
		destReadEndSemaphore == nullptr)
	{
		destAssignSemaphore.reset(new Semaphore);
		destAssignEndSemaphore.reset(new Semaphore);
		destReadSemaphore.reset(new Semaphore);
		destReadEndSemaphore.reset(new Semaphore);
		destAssignSemaphore->openProcessSemaphore(CStringManager::Format("ProcessAssgin_%d", destPid));
		destAssignEndSemaphore->openProcessSemaphore(CStringManager::Format("ProcessAssginEnd_%d", destPid));
		destReadSemaphore->openProcessSemaphore(CStringManager::Format("ProcessRead_%d", destPid));
		destReadEndSemaphore->openProcessSemaphore(CStringManager::Format("ProcessReadEnd_%d", destPid));
		std::pair<int32_t, std::shared_ptr<Semaphore>[4]> pair;
		pair.first = destPid;
		pair.second[0] = destAssignSemaphore;
		pair.second[1] = destAssignEndSemaphore;
		pair.second[2] = destReadSemaphore;
		pair.second[3] = destReadEndSemaphore;
		std::unique_lock<std::mutex> lock(m_sendSemaphoreDequeMutex);
		m_sendSemaphoreDeque.push_back(pair);
	}

	//申请缓存区号
	{
		std::unique_lock<std::mutex> lock(m_assignMutex);
		WriteLock writeLock(*destProcessAssignMutex);
		destAssignSemaphore->processSignal();
		while (!destAssignEndSemaphore->processWait(100))
		{
			if (CSystem::processName(destPid).empty())
			{
				return;
			}
		}
		//读取申请的缓存区号
		assign = (int32_t)(*((int32_t*)area));
	}

	std::string sendMemoryName = CStringManager::Format("ProcessArea_%d_%d", destPid, assign);
	std::shared_ptr<SharedMemory> sharedMemory;
	{
		std::unique_lock<std::mutex> lock(m_sendMemoryDequeMutex);
		for (auto it = m_sendMemoryDeque.begin(); it != m_sendMemoryDeque.end(); ++it)
		{
			if (it->first == sendMemoryName)
			{
				sharedMemory = it->second;
				break;
			}
		}
	}
	if (sharedMemory == nullptr)
	{
		sharedMemory.reset(new SharedMemory(sendMemoryName));
		std::unique_lock<std::mutex> lock(m_sendMemoryDequeMutex);
		m_sendMemoryDeque.push_back(std::pair<std::string, std::shared_ptr<SharedMemory>>(sendMemoryName, sharedMemory));
	}

	void* memory = sharedMemory->writeWithoutLock();
	if (memory == nullptr)
	{
		return;
	}
	*((int32_t*)memory) = m_thisProcessPid;
	*((int32_t*)memory + 1) = length + 4;
	*((int32_t*)memory + 2) = (int32_t)type;
	::memcpy((char*)memory + 12, buffer, length);
	//申请目标读取
	{
		std::unique_lock<std::mutex> lock(m_readMutex);
		WriteLock writeLock(*destProcessReadMutex);
		while (!destReadEndSemaphore->processWait(100))
		{
			if (CSystem::processName(destPid).empty())
			{
				return;
			}
		}
		//把写好的缓存区号写进去
		*((int32_t*)area + 1) = assign;
		destReadSemaphore->processSignal();
	}
}

void ProcessWork::send(const std::string& processName, const char* buffer, int32_t length, MessageType type)
{
	SharedMemory destArea(CStringManager::Format("ProcessArea_%s", processName.c_str()));
	void* pid = destArea.writeWithoutLock();
	if (pid == nullptr)
	{
		return;
	}
	send(*(int32_t*)pid, buffer, length, type);
}

void ProcessWork::post(int32_t destPid, const char* buffer, int32_t length, MessageType type)
{
	if (destPid <= 0 || (buffer == nullptr && length != 0))
	{
		return;
	}
	std::shared_ptr<SendTask> spSendTask(new SendTask);
	spSendTask->setParam(buffer, length, m_thisProcessPid, destPid, "", type);
	std::shared_ptr<CTaskThread> postThread = CTaskThreadManager::Instance().GetThreadInterface(m_postThreadId);
	if (postThread == nullptr)
	{
		return;
	}
	postThread->PostTask(spSendTask);
}

void ProcessWork::post(const std::string& processName, const char* buffer, int32_t length, MessageType type)
{
	if (buffer == nullptr && length != 0)
	{
		return;
	}
	std::shared_ptr<SendTask> spSendTask(new SendTask);
	spSendTask->setParam(buffer, length, m_thisProcessPid, 0, processName, type);
	std::shared_ptr<CTaskThread> postThread = CTaskThreadManager::Instance().GetThreadInterface(m_postThreadId);
	if (postThread == nullptr)
	{
		return;
	}
	postThread->PostTask(spSendTask);
}

void ProcessWork::waitForPostEnd()
{
	if (m_postThreadId == 0)
	{
		return;
	}
	CTaskThreadManager::Instance().WaitForEnd(m_postThreadId);
	m_postThreadId = CTaskThreadManager::Instance().Init();
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