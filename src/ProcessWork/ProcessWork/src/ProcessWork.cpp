#include "ProcessWork.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "ReadTask.h"
#include "SendTask.h"
#include <string.h>
#include "AtomicMath/AtomicMathAPI.h"
#ifdef __unix__
#include <unistd.h>
#endif
#include "CopyTask.h"
#include "ReceiveTask.h"
#include "SendNameTask.h"
#include "ScanTask.h"

ProcessWork::ProcessWork():
m_thisProcessPid(0),
m_readSemaphore(nullptr),
m_areaAssign(nullptr),
m_areaRead(nullptr),
m_pid(nullptr),
m_scanThreadId(0),
m_readThreadId(0),
m_copyThreadId(0),
m_receiveThreadId(0),
m_waitEndPost(false),
m_postThreadId(0),
m_waitEndPostName(false),
m_postNameThreadId(0)
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

bool ProcessWork::initReceive(int32_t receiveSize, int32_t areaCount)
{
	m_areaAssign = new SharedMemory(CStringManager::Format("ProcessAreaAssign_%d", m_thisProcessPid), 4 + areaCount * 2 * 4, false);
	if (m_areaAssign->isFailed())
	{
		delete m_areaAssign;
		m_areaAssign = nullptr;
		return false;
	}
	m_areaAssign->clear();

	m_areaRead = new SharedMemory(CStringManager::Format("ProcessAreaRead_%d", m_thisProcessPid), 4 * 2 + areaCount * 2 * 4, false);
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
	*(int32_t*)areaAssign = areaCount;
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

	m_scanThreadId = CTaskThreadManager::Instance().Init();
	m_readThreadId = CTaskThreadManager::Instance().Init();
	m_copyThreadId = CTaskThreadManager::Instance().Init();
	m_receiveThreadId = CTaskThreadManager::Instance().Init();

	std::shared_ptr<ScanTask> spScanTask(new ScanTask);
	spScanTask->setParam(m_areaAssign);
	CTaskThreadManager::Instance().GetThreadInterface(m_scanThreadId)->PostTask(spScanTask);

	std::shared_ptr<ReceiveTask> spReceiveTask(new ReceiveTask);
	spReceiveTask->setParam(&m_callback, &m_receiveQueue, &m_receiveSemaphore);
	CTaskThreadManager::Instance().GetThreadInterface(m_receiveThreadId)->PostTask(spReceiveTask);

	std::shared_ptr<CopyTask> spCopyTask(new CopyTask);
	spCopyTask->setParam(m_areaAssign, &m_memoryMap, &m_assignQueue, &m_assignSemaphore, &m_receiveQueue, &m_receiveSemaphore);
	CTaskThreadManager::Instance().GetThreadInterface(m_copyThreadId)->PostTask(spCopyTask);

	std::shared_ptr<ReadTask> spReadTask(new ReadTask);
	spReadTask->setParam(m_readSemaphore, areaCount, m_areaRead, &m_assignQueue, &m_assignSemaphore);
	CTaskThreadManager::Instance().GetThreadInterface(m_readThreadId)->PostTask(spReadTask);

	*(int32_t*)pid = m_thisProcessPid;
	
	return true;
}

void ProcessWork::uninitReceive()
{
	if (m_pid != nullptr)
	{
		delete m_pid;
		m_pid = nullptr;
	}

	uninitPostThread();

	if (m_readThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_readThreadId);
		m_readThreadId = 0;
	}
	if (m_copyThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_copyThreadId);
		m_copyThreadId = 0;
	}
	if (m_receiveThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_receiveThreadId);
		m_receiveThreadId = 0;
	}
	if (m_scanThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_scanThreadId);
		m_scanThreadId = 0;
	}

	if (m_areaAssign != nullptr)
	{
		delete m_areaAssign;
		m_areaAssign = nullptr;
	}
	if (m_areaRead != nullptr)
	{
		delete m_areaRead;
		m_areaRead = nullptr;
	}

	m_memoryMap.clear();
	m_callback.clear();
	clear();
	
	if (m_readSemaphore != nullptr)
	{
		m_readSemaphore->closeProcessSemaphore();
		delete m_readSemaphore;
		m_readSemaphore = nullptr;
	}
	
	while (!m_assignQueue.empty())
	{
		int32_t assign = 0;
		m_assignQueue.pop(&assign);
	}
	while (!m_receiveQueue.empty())
	{
		char* receive = nullptr;
		m_receiveQueue.pop(&receive);
		delete[] receive;
	}
}

void ProcessWork::clearUseless()
{
	std::vector<std::string> vecClearName;
	std::vector<int32_t> vecClearPid;
	{
		ReadLock readLock(m_sendNameMemoryMapMutex);
		for (auto itSendName = m_sendNameMemoryMap.begin(); itSendName != m_sendNameMemoryMap.end(); ++itSendName)
		{
			void* pid = itSendName->second.second->writeWithoutLock();
			if (pid == nullptr)
			{
				printf("sendNameMemory nullptr, name = %s", itSendName->first.c_str());
				continue;
			}
			int32_t oldDestPid = itSendName->second.first;
			int32_t destPid = *(int32_t*)pid;
			if (destPid != oldDestPid)
			{
				//把旧的pid放到清空队列
				vecClearPid.push_back(oldDestPid);
				//把新的pid放到map中
				itSendName->second.first = destPid;
			}
			//如果新的pid也不存在
			if (CSystem::processName(destPid).empty())
			{
				//把新的pid放到清空队列
				vecClearPid.push_back(destPid);
				//把当前节点的名字放到清空队列
				vecClearName.push_back(itSendName->first);
			}
		}
	}
	
	int32_t index = -1;
	while (index++ != vecClearName.size() - 1)
	{
		WriteLock writeLock(m_sendNameMemoryMapMutex);
		auto itSendName = m_sendNameMemoryMap.find(vecClearName[index]);
		if (itSendName != m_sendNameMemoryMap.end())
		{
			m_sendNameMemoryMap.erase(itSendName);
		}
	}

	index = -1;
	while (index++ != vecClearPid.size() - 1)
	{
		clearDestPid(vecClearPid[index]);
	}
}

void ProcessWork::clear()
{
	{
		WriteLock writeLock(m_sendProcessAssignMapMutex);
		m_sendProcessAssignMap.clear();
	}
	{
		WriteLock writeLock(m_sendProcessReadMapMutex);
		m_sendProcessReadMap.clear();
	}
	{
		WriteLock writeLock(m_sendMemoryMapMutex);
		m_sendMemoryMap.clear();
	}
	{
		WriteLock writeLock(m_sendSemaphoreMapMutex);
		m_sendSemaphoreMap.clear();
	}
	{
		WriteLock writeLock(m_sendNameMemoryMapMutex);
		m_sendNameMemoryMap.clear();
	}
}

void ProcessWork::initPostThread()
{
	if (m_postThreadId != 0)
	{
		return;
	}

	m_waitEndPost = false;
	m_postThreadId = CTaskThreadManager::Instance().Init();
	std::shared_ptr<SendTask> spSendTask(new SendTask);
	spSendTask->setParam(&m_waitEndPost, &m_postQueue, &m_postSemaphore);
	CTaskThreadManager::Instance().GetThreadInterface(m_postThreadId)->PostTask(spSendTask);

	m_waitEndPostName = false;
	m_postNameThreadId = CTaskThreadManager::Instance().Init();
	std::shared_ptr<SendNameTask> spSendNameTask(new SendNameTask);
	spSendNameTask->setParam(&m_waitEndPostName, &m_postNameQueue, &m_postNameSemaphore);
	CTaskThreadManager::Instance().GetThreadInterface(m_postNameThreadId)->PostTask(spSendNameTask);
}

void ProcessWork::uninitPostThread()
{
	if (m_postThreadId == 0)
	{
		return;
	}

	CTaskThreadManager::Instance().Uninit(m_postThreadId);
	m_postThreadId = 0;
	m_waitEndPost = false;
	while (!m_postQueue.empty())
	{
		char* postBuffer = nullptr;
		m_postQueue.pop(&postBuffer);
		delete[] postBuffer;
	}

	CTaskThreadManager::Instance().Uninit(m_postNameThreadId);
	m_postNameThreadId = 0;
	m_waitEndPostName = false;
	while (!m_postNameQueue.empty())
	{
		char* postNameBuffer = nullptr;
		m_postNameQueue.pop(&postNameBuffer);
		delete[] postNameBuffer;
	}
}

//可以将申请的缓冲区号改为先分配号一组让发送者有序拿，然后再把写好的分配号放入申请读取组中增加效率，读取端将信号量改为事件，拷贝出分配号一次性读取
//效率可以优化到一秒25万左右
void ProcessWork::send(int32_t destPid, const char* buffer, int32_t length, MessageType type)
{
	if (destPid <= 0 || (buffer == nullptr && length != 0) || (int32_t)type < 0)
	{
		return;
	}

	std::shared_ptr<SharedMemory> destAreaAssign;
	{
		ReadLock readLock(m_sendProcessAssignMapMutex);
		auto itAssign = m_sendProcessAssignMap.find(destPid);
		if (itAssign != m_sendProcessAssignMap.end())
		{
			destAreaAssign = itAssign->second;
		}
	}
	if (destAreaAssign == nullptr)
	{
		WriteLock writeLock(m_sendProcessAssignMapMutex);
		destAreaAssign.reset(new SharedMemory(CStringManager::Format("ProcessAreaAssign_%d", destPid)));
		m_sendProcessAssignMap[destPid] = destAreaAssign;
	}

	void* areaAssign = destAreaAssign->writeWithoutLock();
	if (areaAssign == nullptr)
	{
		return;
	}

	std::shared_ptr<SharedMemory> destAreaRead;
	{
		ReadLock readLock(m_sendProcessReadMapMutex);
		auto itReadMemory = m_sendProcessReadMap.find(destPid);
		if (itReadMemory != m_sendProcessReadMap.end())
		{
			destAreaRead = itReadMemory->second;
		}
	}
	if (destAreaRead == nullptr)
	{
		WriteLock writeLock(m_sendProcessReadMapMutex);
		destAreaRead.reset(new SharedMemory(CStringManager::Format("ProcessAreaRead_%d", destPid)));
		m_sendProcessReadMap[destPid] = destAreaRead;
	}

	void* areaRead = destAreaRead->writeWithoutLock();
	if (areaRead == nullptr)
	{
		return;
	}

	std::shared_ptr<Semaphore> destReadSemaphore;
	{
		ReadLock readLock(m_sendSemaphoreMapMutex);
		auto itReadSemaphore = m_sendSemaphoreMap.find(destPid);
		if (itReadSemaphore != m_sendSemaphoreMap.end())
		{
			destReadSemaphore = itReadSemaphore->second;
		}
	}
	if (destReadSemaphore == nullptr)
	{
		WriteLock writeLock(m_sendSemaphoreMapMutex);
		destReadSemaphore.reset(new Semaphore);
		destReadSemaphore->openProcessSemaphore(CStringManager::Format("ProcessRead_%d", destPid));
		m_sendSemaphoreMap[destPid] = destReadSemaphore;
	}

	int32_t areaCount = *(int32_t*)areaAssign;
	int32_t assign = 0;
	//申请缓存区号
	{
		int32_t searchCount = 0;
		while (true)
		{
			int32_t index = -1;
			while (index++ != areaCount - 1)
			{
				if (AtomicMath::compareAndSwap((int32_t*)areaAssign + 1 + index * 2 + 1, 0, m_thisProcessPid))
				{
					//读取申请的缓存区号
					assign = *((int32_t*)areaAssign + 1 + index * 2);
					break;
				}
			}
			if (assign != 0)
			{
				break;
			}
			++searchCount;
			if ((searchCount % 10 == 0) && CSystem::processName(destPid).empty())
			{
				clearUseless();
				break;
			}
			//RCSend("cannot has assign");
		}
	}

	if (assign == 0)
	{
		return;
	}

	std::string sendMemoryName = CStringManager::Format("ProcessArea_%d_%d", destPid, assign);

	std::shared_ptr<SharedMemory> sharedMemory;
	{
		ReadLock readLock(m_sendMemoryMapMutex);
		auto itMemory = m_sendMemoryMap.find(sendMemoryName);
		if (itMemory != m_sendMemoryMap.end())
		{
			sharedMemory = itMemory->second;
		}
	}
	if (sharedMemory == nullptr)
	{
		WriteLock writeLock(m_sendMemoryMapMutex);
		sharedMemory.reset(new SharedMemory(sendMemoryName));
		m_sendMemoryMap[sendMemoryName] = sharedMemory;
	}

	void* memory = sharedMemory->writeWithoutLock();
	if (memory == nullptr)
	{
		printf("memory nullptr\n");
		return;
	}
	*((int32_t*)memory) = m_thisProcessPid;
	*((int32_t*)memory + 1) = (int32_t)type;
	*((int32_t*)memory + 2) = length;
	if (length != 0)
	{
		::memcpy((char*)memory + 12, buffer, length);
	}

	//申请目标读取
	{
		int32_t& write = *(int32_t*)areaRead;
		int32_t* beginReadPtr = (int32_t*)areaRead + 2;
		int32_t writeIndex = write % areaCount;
		if (writeIndex < 0)
		{
			writeIndex += areaCount;
		}
		while (true)
		{
			//前4字节为0，后4字节拿到1
			if ((*(beginReadPtr + writeIndex * 2) == 0) && (AtomicMath::compareAndSwap(beginReadPtr + writeIndex * 2 + 1, 0, m_thisProcessPid)))
			{
				//把写好的缓存区号写进去
				*(beginReadPtr + writeIndex * 2) = assign;
				//局部会出现write值被旧值覆盖，但出现新的send后会重新写入正确的值，为了防止抢占到之后崩溃
				//不可以写write = writeIndex + 1;因为会出现前面的后写入，导致存储顺序出错
				//0 0  0 0  0 0  0 0  0 2145  0 7896  0 5468  12 5489，第五第六格写入通知，但是第五格后写入
				//一旦在这里崩溃无法保证后续几个包之内的顺序，且会出现少通知最后一个包的情况
				int32_t currentWrite = AtomicMath::selfAddOne(&write);
				if (currentWrite >= areaCount)
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
		destReadSemaphore->processSignal();
	}
}

void ProcessWork::send(const std::string& processName, const char* buffer, int32_t length, MessageType type)
{
	int32_t oldDestPid = 0;
	std::shared_ptr<SharedMemory> destArea;
	{
		ReadLock readLock(m_sendNameMemoryMapMutex);
		auto itSendName = m_sendNameMemoryMap.find(processName);
		if (itSendName != m_sendNameMemoryMap.end())
		{
			oldDestPid = itSendName->second.first;
			destArea = itSendName->second.second;
		}
	}

	void* pid = nullptr;
	if (destArea == nullptr)
	{
		destArea.reset(new SharedMemory(CStringManager::Format("ProcessArea_%s", processName.c_str())));
		pid = destArea->writeWithoutLock();
		if (pid == nullptr)
		{
			return;
		}
		oldDestPid = *(int32_t*)pid;
		WriteLock writeLock(m_sendNameMemoryMapMutex);
		m_sendNameMemoryMap[processName].first = *(int32_t*)pid;
		m_sendNameMemoryMap[processName].second = destArea;
	}

	pid = destArea->writeWithoutLock();
	int32_t destPid = *(int32_t*)pid;
	if (destPid != oldDestPid)
	{
		{
			ReadLock readLock(m_sendNameMemoryMapMutex);
			auto itSendName = m_sendNameMemoryMap.find(processName);
			if (itSendName != m_sendNameMemoryMap.end())
			{
				itSendName->second.first = destPid;
			}
		}
		clearDestPid(oldDestPid);
	}

	send(destPid, buffer, length, type);
}

void ProcessWork::send(int32_t destPid, const std::string& message, MessageType type)
{
	send(destPid, message.empty() ? nullptr : message.c_str(), message.size(), type);
}

void ProcessWork::send(const std::string& processName, const std::string& message, MessageType type)
{
	send(processName, message.empty() ? nullptr : message.c_str(), message.size(), type);
}

void ProcessWork::post(int32_t destPid, const char* buffer, int32_t length, MessageType type)
{
	if (destPid <= 0 || (buffer == nullptr && length != 0) || (int32_t)type < 0)
	{
		return;
	}

	char* postBuffer = new char[12 + length];
	*(int32_t*)postBuffer = destPid;
	*((int32_t*)postBuffer + 1) = type;
	*((int32_t*)postBuffer + 2) = length;
	if (length != 0)
	{
		::memcpy(postBuffer + 12, buffer, length);
	}

	m_postQueue.push(postBuffer);
	m_postSemaphore.signal();
}

void ProcessWork::post(const std::string& processName, const char* buffer, int32_t length, MessageType type)
{
	if (processName.empty() || (buffer == nullptr && length != 0) || (int32_t)type < 0)
	{
		return;
	}

	int32_t nameLength = strlen(processName.c_str());
	char* postNameBuffer = new char[nameLength + 1 + 8 + length];
	::memcpy(postNameBuffer, processName.c_str(), nameLength + 1);
	*((int32_t*)(postNameBuffer + nameLength + 1)) = type;
	*((int32_t*)(postNameBuffer + nameLength + 1) + 1) = length;
	if (length != 0)
	{
		::memcpy(postNameBuffer + nameLength + 1 + 8, buffer, length);
	}

	m_postNameQueue.push(postNameBuffer);
	m_postNameSemaphore.signal();
}

void ProcessWork::post(int32_t destPid, const std::string& message, MessageType type)
{
	post(destPid, message.empty() ? nullptr : message.c_str(), message.size(), type);
}

void ProcessWork::post(const std::string& processName, const std::string& message, MessageType type)
{
	post(processName, message.empty() ? nullptr : message.c_str(), message.size(), type);
}

void ProcessWork::waitForPostEnd()
{
	if (m_postThreadId == 0)
	{
		return;
	}
	m_waitEndPost = true;
	m_postSemaphore.signal();
	CTaskThreadManager::Instance().WaitForEnd(m_postThreadId);
	m_postThreadId = 0;
	m_waitEndPostName = true;
	m_postNameSemaphore.signal();
	CTaskThreadManager::Instance().WaitForEnd(m_postNameThreadId);
	m_postNameThreadId = 0;
	initPostThread();
}

void ProcessWork::clearDestPid(int32_t destPid)
{
	{
		WriteLock writeLock(m_sendProcessAssignMapMutex);
		auto it = m_sendProcessAssignMap.find(destPid);
		if (it != m_sendProcessAssignMap.end())
		{
			m_sendProcessAssignMap.erase(it);
		}
	}
	{
		WriteLock writeLock(m_sendProcessReadMapMutex);
		auto it = m_sendProcessReadMap.find(destPid);
		if (it != m_sendProcessReadMap.end())
		{
			m_sendProcessReadMap.erase(it);
		}
	}
	{
		std::string sendMemoryNameKey = CStringManager::Format("ProcessArea_%d_", destPid);
		WriteLock writeLock(m_sendMemoryMapMutex);
		for (auto it = m_sendMemoryMap.begin(); it != m_sendMemoryMap.end();)
		{
			if (it->first.find(sendMemoryNameKey) != -1)
			{
				it = m_sendMemoryMap.erase(it);
				continue;
			}
			++it;
		}
	}
	{
		WriteLock writeLock(m_sendSemaphoreMapMutex);
		auto it = m_sendSemaphoreMap.find(destPid);
		if (it != m_sendSemaphoreMap.end())
		{
			m_sendSemaphoreMap.erase(it);
		}
	}
}

//#include "ProcessWork/ProcessWorkAPI.h"
//#include "CSystem/CSystemAPI.h"
//#include "CStringManager/CStringManagerAPI.h"
//#include "LogManager/LogManagerAPI.h"
//
//int32_t num = 0;
//
//class Receive : public ProcessReceiveCallback
//{
//public:
//	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
//	{
//		LOGINFO("num = %d, %s", num, buffer);
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
//	ProcessWork::instance().addProcessReceiveCallback(&receive);
//	ProcessWork::instance().initReceive(1024, 1000);
//	Sleep(20000);
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
//	RCSend("send end");
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