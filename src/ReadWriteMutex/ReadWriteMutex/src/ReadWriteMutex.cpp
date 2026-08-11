#include "ReadWriteMutex.h"
#include <stdio.h>

ReadWriteMutex::ReadWriteMutex()
#ifdef __SUPPORT_XP__
	:m_readCount(0)
#endif
{
#ifdef _MSC_VER
#ifndef __SUPPORT_XP__
	InitializeSRWLock(&m_writeMutex);
#endif
#elif __unix__
	int result = pthread_rwlock_init(&m_writeMutex, nullptr);
	if (result != 0)
	{
		printf("pthread_rwlock_init error = %d\n", result);
	}
#endif
}

#ifdef __unix__
ReadWriteMutex::~ReadWriteMutex()
{
	int result = pthread_rwlock_destroy(&m_writeMutex);
	if (result != 0)
	{
		printf("pthread_rwlock_destroy error = %d\n", result);
	}
}
#endif

void ReadWriteMutex::read()
{
#ifdef _MSC_VER
#ifdef __SUPPORT_XP__
	std::unique_lock<std::mutex> lock(m_writeMutex);
	while (m_readCount < 0)
	{
		m_condition.wait(lock);
	}
	++m_readCount;
#else
	AcquireSRWLockShared(&m_writeMutex);
#endif
#elif __unix__
	int result = pthread_rwlock_rdlock(&m_writeMutex);
	if (result != 0)
	{
		printf("pthread_rwlock_rdlock error = %d\n", result);
	}
#endif
}

void ReadWriteMutex::unread()
{
#ifdef _MSC_VER
#ifdef __SUPPORT_XP__
	std::unique_lock<std::mutex> lock(m_writeMutex);
	if (--m_readCount == 0)
	{
		//叫醒一个等待的写操作
		m_condition.notify_one();
	}
#else
	ReleaseSRWLockShared(&m_writeMutex);
#endif
#elif __unix__
	int result = pthread_rwlock_unlock(&m_writeMutex);
	if (result != 0)
	{
		printf("pthread_rwlock_unlock error = %d\n", result);
	}
#endif
}

void ReadWriteMutex::write()
{
#ifdef _MSC_VER
#ifdef __SUPPORT_XP__
	std::unique_lock<std::mutex> lock(m_writeMutex);
	while (m_readCount != 0)
	{
		m_condition.wait(lock);
	}
	m_readCount = -1;
#else
	AcquireSRWLockExclusive(&m_writeMutex);
#endif
#elif __unix__
	int result = pthread_rwlock_wrlock(&m_writeMutex);
	if (result != 0)
	{
		printf("pthread_rwlock_wrlock error = %d\n", result);
	}
#endif
}

void ReadWriteMutex::unwrite()
{
#ifdef _MSC_VER
#ifdef __SUPPORT_XP__
	std::unique_lock<std::mutex> lock(m_writeMutex);
	m_readCount = 0;
	//叫醒所有等待的读和写操作
	m_condition.notify_all();
#else
	ReleaseSRWLockExclusive(&m_writeMutex);
#endif
#elif __unix__
	int result = pthread_rwlock_unlock(&m_writeMutex);
	if (result != 0)
	{
		printf("pthread_rwlock_unlock error = %d\n", result);
	}
#endif
}

//#include "FileReadWriteMutex.h"
//#include "ProcessReadWriteMutex.h"
//#include "ReadLock.h"
//#include "RecursiveReadWriteMutex.h"
//#include "WriteLock.h"
//#include <atomic>
//#include <cstdlib>
//#include <functional>
//#include <iostream>
//#include <stdint.h>
//#include <string>
//#include <thread>
//#include <vector>
//
//int main(int32_t argc, char** argv)
//{
//	int32_t totalCount = 0;
//	int32_t failCount = 0;
//	int32_t skipCount = 0;
//	std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
//	{
//		++totalCount;
//		std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
//		if (!ok)
//		{
//			++failCount;
//		}
//	};
//	std::function<void(const std::string&)> skip = [&skipCount](const std::string& name) -> void
//	{
//		++skipCount;
//		std::cout << "[SKIP] " << name << std::endl;
//	};
//
//#ifdef _MSC_VER
//	if (argc >= 3 && std::string(argv[1]) == "--process-child")
//	{
//		ProcessReadWriteMutex processMutex(argv[2]);
//		bool locked = processMutex.trywrite();
//		if (locked)
//		{
//			processMutex.unwrite();
//			return 1;
//		}
//		return 0;
//	}
//#else
//	(void)argc;
//	(void)argv;
//#endif
//
//	ReadWriteMutex mutex;
//	mutex.read();
//	mutex.unread();
//	mutex.write();
//	mutex.unwrite();
//	check(true, "ReadWriteMutex basic read write lock");
//	{
//		ReadLock readLock(mutex);
//	}
//	{
//		WriteLock writeLock(mutex);
//	}
//	check(true, "ReadWriteMutex RAII lock");
//
//	RecursiveReadWriteMutex recursiveMutex;
//	recursiveMutex.write();
//	recursiveMutex.write();
//	check(recursiveMutex.lockCount() == 2, "RecursiveReadWriteMutex recursive write count");
//	recursiveMutex.unwrite();
//	recursiveMutex.unwrite();
//	check(recursiveMutex.lockCount() == 0, "RecursiveReadWriteMutex recursive release count");
//
//	ReadWriteMutex stressMutex;
//	int32_t value = 0;
//	std::vector<std::thread> threads;
//	for (int32_t threadIndex = 0; threadIndex < 8; ++threadIndex)
//	{
//		threads.push_back(std::thread([&stressMutex, &value]() -> void
//		{
//			for (int32_t loopIndex = 0; loopIndex < 5000; ++loopIndex)
//			{
//				WriteLock writeLock(stressMutex);
//				++value;
//			}
//		}));
//	}
//	for (size_t i = 0; i < threads.size(); ++i)
//	{
//		threads[i].join();
//	}
//	check(value == 40000, "ReadWriteMutex multithread write pressure");
//
//	FileReadWriteMutex fileMutex("ReadWriteMutexTestFile");
//	fileMutex.write();
//	fileMutex.unwrite();
//	check(true, "FileReadWriteMutex basic write lock");
//
//#ifdef _MSC_VER
//	std::string processMutexName = "ReadWriteMutexProcessTest";
//	ProcessReadWriteMutex processMutex(processMutexName);
//	processMutex.write();
//	std::string exePath = argc > 0 ? argv[0] : "";
//	std::string command = "\"" + exePath + "\" --process-child " + processMutexName;
//	int32_t processResult = std::system(command.c_str());
//	processMutex.unwrite();
//	check(processResult == 0, "ProcessReadWriteMutex multiprocess trywrite pressure");
//#else
//	skip("ProcessReadWriteMutex multiprocess trywrite pressure, current platform has no bool trywrite");
//#endif
//
//	std::cout << "ReadWriteMutex test " << (failCount == 0 ? "PASS" : "FAIL") << ", total=" << totalCount << ", failed=" << failCount << ", skipped=" << skipCount << std::endl;
//	return failCount == 0 ? 0 : 1;
//}