#include "ProcessWork.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "ReadTask.h"
#include "SendTask.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include <string.h>
#include "AtomicMath/AtomicMathAPI.h"
#ifdef __unix__
#include <unistd.h>
#endif

ProcessWork::ProcessWork():
m_thisProcessPid(0),
m_readSemaphore(nullptr),
m_areaAssign(nullptr),
m_areaRead(nullptr),
m_pid(nullptr),
m_readThreadId(0),
m_copyThreadId(0),
m_receiveThreadId(0),
m_postThreadId(0),
m_areaCount(0),
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
	m_areaCount = areaCount;
	m_flow = flow;
	m_sendProcessAssignDeque.setFinite(m_flow);
	m_sendMemoryDeque.setFinite(m_areaCount * m_flow);
	m_sendSemaphoreDeque.setFinite(m_flow);

	m_areaAssign = new SharedMemory(CStringManager::Format("ProcessAreaAssign_%d", m_thisProcessPid), 4 + m_areaCount * 2 * 4, false);
	if (m_areaAssign->isFailed())
	{
		delete m_areaAssign;
		m_areaAssign = nullptr;
		return false;
	}
	m_areaAssign->clear();

	m_areaRead = new SharedMemory(CStringManager::Format("ProcessAreaRead_%d", m_thisProcessPid), 4 * 2 + m_areaCount * 2 * 4, false);
	if (m_areaRead->isFailed())
	{
		delete m_areaAssign;
		m_areaAssign = nullptr;
		delete m_areaRead;
		m_areaRead = nullptr;
		return false;
	}
	m_areaRead->clear();

#ifdef _MSC_VER
	std::string currentExeName = CSystem::GetCurrentExeName();
#elif __unix__
	std::string currentExeName = CSystem::GetCurrentExeFullName();
#endif
	m_pid = new SharedMemory(CStringManager::Format("ProcessArea_%s", currentExeName.c_str()), 4, true);
	if (m_pid->isFailed())
	{
		delete m_areaAssign;
		m_areaAssign = nullptr;
		delete m_areaRead;
		m_areaRead = nullptr;
		delete m_pid;
		m_pid = nullptr;
		return false;
	}
	void* pid = m_pid->writeWithoutLock();
	if (pid == nullptr)
	{
		printf("init pid nullptr\n");
	}

	m_readSemaphore = new Semaphore;

	m_readSemaphore->createProcessSemaphore(CStringManager::Format("ProcessRead_%d", m_thisProcessPid));

	void* areaAssign = m_areaAssign->writeWithoutLock();
	*(int32_t*)areaAssign = m_areaCount;
	int32_t memoryIndex = 0;
	int32_t index = -1;
	while (index++ != areaCount - 1)
	{
		std::shared_ptr<SharedMemory> spSharedMemory;
		do
		{
			spSharedMemory.reset(new SharedMemory(CStringManager::Format("ProcessArea_%d_%d", m_thisProcessPid, ++memoryIndex), receiveSize, false));
		} while (spSharedMemory->isFailed());
		*((int32_t*)areaAssign + 1 + index * 2) = memoryIndex;
		m_memoryMap[memoryIndex] = spSharedMemory;
	}

	m_readThreadId = CTaskThreadManager::Instance().Init();
	m_copyThreadId = CTaskThreadManager::Instance().Init();
	m_receiveThreadId = CTaskThreadManager::Instance().Init();

	std::shared_ptr<ReadTask> spReadTask(new ReadTask);
	spReadTask->setParam(&m_callback,
		m_readSemaphore,
		m_areaAssign,
		m_areaRead,
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

	CTaskThreadManager::Instance().Uninit(m_readThreadId);
	CTaskThreadManager::Instance().Uninit(m_copyThreadId);
	CTaskThreadManager::Instance().Uninit(m_receiveThreadId);
	m_memoryMap.clear();

	m_readSemaphore->closeProcessSemaphore();

	delete m_readSemaphore;
	delete m_areaAssign;
	delete m_areaRead;

	m_readSemaphore = nullptr;
	m_areaAssign = nullptr;
	m_areaRead = nullptr;

	m_readThreadId = 0;
	m_copyThreadId = 0;
	m_receiveThreadId = 0;
	m_callback.clear();
}

void ProcessWork::clear()
{
	{
		std::unique_lock<std::mutex> lock(m_sendProcessAssignDequeMutex);
		m_sendProcessAssignDeque.clear();
	}
	{
		std::unique_lock<std::mutex> lock(m_sendProcessReadDequeMutex);
		m_sendProcessReadDeque.clear();
	}
	{
		std::unique_lock<std::mutex> lock(m_sendMemoryDequeMutex);
		m_sendMemoryDeque.clear();
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
	
	std::shared_ptr<SharedMemory> destAreaAssign;
	{
		std::unique_lock<std::mutex> lock(m_sendProcessAssignDequeMutex);
		for (auto it = m_sendProcessAssignDeque.begin(); it != m_sendProcessAssignDeque.end(); ++it)
		{
			if (it->first == destPid)
			{
				destAreaAssign = it->second;
				break;
			}
		}
	}
	if (destAreaAssign == nullptr)
	{
		destAreaAssign.reset(new SharedMemory(CStringManager::Format("ProcessAreaAssign_%d", destPid)));
		std::unique_lock<std::mutex> lock(m_sendProcessAssignDequeMutex);
		m_sendProcessAssignDeque.push_back(std::pair<int32_t, std::shared_ptr<SharedMemory>>(destPid, destAreaAssign));
	}
	
	void* areaAssign = destAreaAssign->writeWithoutLock();
	if (areaAssign == nullptr)
	{
		return;
	}
	
	std::shared_ptr<SharedMemory> destAreaRead;
	{
		std::unique_lock<std::mutex> lock(m_sendProcessReadDequeMutex);
		for (auto it = m_sendProcessReadDeque.begin(); it != m_sendProcessReadDeque.end(); ++it)
		{
			if (it->first == destPid)
			{
				destAreaRead = it->second;
				break;
			}
		}
	}
	if (destAreaRead == nullptr)
	{
		destAreaRead.reset(new SharedMemory(CStringManager::Format("ProcessAreaRead_%d", destPid)));
		std::unique_lock<std::mutex> lock(m_sendProcessReadDequeMutex);
		m_sendProcessReadDeque.push_back(std::pair<int32_t, std::shared_ptr<SharedMemory>>(destPid, destAreaRead));
	}
	
	void* areaRead = destAreaRead->writeWithoutLock();
	if (areaRead == nullptr)
	{
		return;
	}
	
	std::shared_ptr<Semaphore> destReadSemaphore;
	{
		std::unique_lock<std::mutex> lock(m_sendSemaphoreDequeMutex);
		for (auto it = m_sendSemaphoreDeque.begin(); it != m_sendSemaphoreDeque.end(); ++it)
		{
			if (it->first == destPid)
			{
				destReadSemaphore = it->second;
				break;
			}
		}
	}
	
	if (destReadSemaphore == nullptr)
	{
		destReadSemaphore.reset(new Semaphore);
		destReadSemaphore->openProcessSemaphore(CStringManager::Format("ProcessRead_%d", destPid));
		std::pair<int32_t, std::shared_ptr<Semaphore>> pair;
		pair.first = destPid;
		pair.second = destReadSemaphore;
		std::unique_lock<std::mutex> lock(m_sendSemaphoreDequeMutex);
		m_sendSemaphoreDeque.push_back(pair);
	}
	
	int32_t areaCount = *(int32_t*)areaAssign;
	int32_t assign = 0;
	//申请缓存区号
	{
		do 
		{
			int32_t index = -1;
			while (index++ != areaCount - 1)
			{
				int32_t& areaOwn = *((int32_t*)areaAssign + 1 + index * 2 + 1);
				if (areaOwn == 0)
				{
					if (AtomicMath::selfAddOne(&areaOwn) == 1)
					{
						//读取申请的缓存区号
						assign = *((int32_t*)areaAssign + 1 + index * 2);
						break;
					}
					break;
				}
			}
			if (assign != 0)
			{
				break;
			}
		} while (!CSystem::processName(destPid).empty());
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
		int32_t& write = *(int32_t*)areaRead;
		int32_t* beginReadPtr = (int32_t*)areaRead + 2;
		int32_t writeIndex = write % areaCount;
		if (writeIndex < 0)
		{
			writeIndex += areaCount;
		}
		//int32_t errorBeginTime = ::GetTickCount();
		while (true)
		{
			//前4字节为0，后4字节拿到1
			if ((*(beginReadPtr + writeIndex * 2) == 0) && (AtomicMath::selfAddOne(beginReadPtr + writeIndex * 2 + 1) == 1))
			{
				//把写好的缓存区号写进去
				*(beginReadPtr + writeIndex * 2) = assign;
				//局部会出现write值被旧值覆盖，但出现新的send后会重新写入正确的值，为了防止抢占到之后崩溃
				write = writeIndex + 1;
				if (write >= areaCount)
				{
					AtomicMath::selfSub(&write, areaCount);
				}
				break;
			}
			++writeIndex;
			if (writeIndex == areaCount)
			{
				writeIndex = 0;
			}
		}
		//if (::GetTickCount() - errorBeginTime != 0)
		//{
		//	RCSend("errorCount = %d", ::GetTickCount() - errorBeginTime);
		//}
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
//	void receive(int32_t sendPid, char* buffer, int32_t length, int32_t type)
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