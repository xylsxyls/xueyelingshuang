#ifndef __MEMORY_TEST_H__
#define __MEMORY_TEST_H__

#include <map>
#include <unordered_map>
#include <atomic>

#ifdef _MSC_VER
#include <windows.h>
#include <dbghelp.h>
#include "D:\\SendToMessageTest.h"
#pragma comment(lib, "Dbghelp.lib")
#elif __unix__
#include <thread>
#include <execinfo.h>
#include <string.h>
#include <pthread.h>
#include "SendToMessageTest.h"
#endif

#define MEMORY_LOG_FILE
//#define MEMORY_LIST
#ifdef MEMORY_LOG_FILE
#define MEMORY_LOG_FILE_NAME "MemoryTest.log"
#endif

#define MEMORY_PICTURE(id) MemoryTestTrace::instance().memoryPicture(id)
#define MEMORY_STACK_PICTURE(id) MemoryTestTrace::instance().memoryStackPicture(id)

class MemoryRecursiveMutex
{
public:
	MemoryRecursiveMutex():
		m_lockCount(0)
	{
#ifdef _MSC_VER
		InitializeCriticalSection(&m_lock);
#elif __unix__
		pthread_mutexattr_init(&m_attr);
		pthread_mutexattr_settype(&m_attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&m_lock, &m_attr);
#endif
	}

	~MemoryRecursiveMutex()
	{
#ifdef _MSC_VER
		DeleteCriticalSection(&m_lock);
#elif __unix__
		pthread_mutex_destroy(&m_lock);
		pthread_mutexattr_destroy(&m_attr);
#endif
	}

	void lock()
	{
#ifdef _MSC_VER
		EnterCriticalSection(&m_lock);
#elif __unix__
		pthread_mutex_lock(&m_lock);
#endif
		++m_lockCount;
	}

	void unlock()
	{
		--m_lockCount;
#ifdef _MSC_VER
		LeaveCriticalSection(&m_lock);
#elif __unix__
		pthread_mutex_unlock(&m_lock);
#endif
	}

	int lockCount()
	{
		return m_lockCount;
	}

private:
#ifdef _MSC_VER
	CRITICAL_SECTION m_lock;
#elif __unix__
	pthread_mutex_t m_lock;
	pthread_mutexattr_t m_attr;
#endif
	int m_lockCount;
};

class MemoryUniqueLock
{
public:
	MemoryUniqueLock(MemoryRecursiveMutex& mutex):
		m_mutex(mutex)
	{
		m_mutex.lock();
	}

	~MemoryUniqueLock()
	{
		m_mutex.unlock();
	}

private:
	MemoryRecursiveMutex& m_mutex;
};

struct PrintEntry
{
	void* m_p;
	const char* m_file;
	int32_t m_line;
	size_t m_size;
	size_t m_totalSize;
	size_t m_entrySize;
	std::string m_stack;
	bool m_isPrintf;
	bool m_isRCSend;
	bool m_isfPrintf;
	std::string m_preText;
	std::string m_nextText;

	PrintEntry():
		m_p(nullptr),
		m_file(nullptr),
		m_line(-1),
		m_size(-1),
		m_totalSize(-1),
		m_entrySize(-1),
		m_isPrintf(false),
		m_isRCSend(false),
		m_isfPrintf(false)
	{
		
	}

	std::string printStr()
	{
		m_printStr.clear();
		if (m_isPrintf || m_isRCSend || m_isfPrintf)
		{
			m_printStr = m_preText;
			std::string pstr;
#ifdef _MSC_VER
			pstr = "0x%p";
#elif __unix__
			pstr = "%p";
#endif
			if (m_p != nullptr)
			{
				appendEntry();
				m_printStr.append("p = " + pstr);
			}
			if (m_file != nullptr)
			{
				appendEntry();
				m_printStr.append("file = %s");
			}
			std::string sizeStr;
#if defined _WIN64 || defined __x86_64__
			sizeStr = "%I64d";
#else
			sizeStr = "%d";
#endif
			if (m_line != -1)
			{
				appendEntry();
				m_printStr.append("line = " + sizeStr);
			}
			
			if (m_size != -1)
			{
				appendEntry();
				m_printStr.append("size = " + sizeStr);
			}
			
			if (m_totalSize != -1)
			{
				appendEntry();
				m_printStr.append("totalSize = " + sizeStr);
			}
			
			if (m_entrySize != -1)
			{
				appendEntry();
				m_printStr.append("entrySize = " + sizeStr);
			}
			if (!m_nextText.empty())
			{
				appendEntry();
				m_printStr.append(m_nextText);
			}
			if (!m_stack.empty())
			{
				if (!m_printStr.empty())
				{
					m_printStr.push_back('\n');
				}
				m_printStr.append("%s");
			}
			if (m_isPrintf)
			{
				m_printStr.push_back('\n');
			}
			if (m_isfPrintf)
			{
				m_printStr.push_back('\n');
			}
		}
		return m_printStr;
	}

	void appendEntry()
	{
		if (m_printStr.empty())
		{
			return;
		}
		m_printStr.append(", ");
	}

private:
	std::string m_printStr;
};

class MemoryEntry
{
public:
	MemoryEntry(const std::string& stack, const char* file, int line):
		m_stack(stack),
		m_file(file),
		m_line(line),
		m_size(0)
	{

	}
	MemoryEntry(const std::string& stack, const char* file, int line, size_t size):
		m_stack(stack),
		m_file(file),
		m_line(line),
		m_size(size)
	{

	}

	MemoryEntry():
		m_file(nullptr),
		m_line(0),
		m_size(0)
	{

	}

	std::string stack() const
	{
		return m_stack;
	}

	const char* file() const
	{
		return m_file;
	}

	int line() const
	{
		return m_line;
	}

	size_t size() const
	{
		return m_size;
	}

private:
	std::string m_stack;
	const char* m_file;
	int m_line;
	size_t m_size;
};

class MemoryReady
{
private:
	MemoryReady()
	{
		m_isReady = false;
	}

public:
	static MemoryReady& instance()
	{
		static MemoryReady s_memoryReady;
		return s_memoryReady;
	}

	void init()
	{
		m_isReady = true;
	}

	void uninit()
	{
		m_isReady = false;
	}

	bool isReady()
	{
		return m_isReady;
	}

private:
	std::atomic<bool> m_isReady;
};

class MemoryTestTrace
{
private:
	MemoryTestTrace():
		m_totalSize(0)
#if defined(MEMORY_LOG_FILE)
		, m_logfile(nullptr)
#endif
	{
#if defined(MEMORY_LOG_FILE)
		m_logfile = fopen((GetCurrentExePath() + GetCurrentExeName() + MEMORY_LOG_FILE_NAME).c_str(), "wt");
		if (m_logfile == nullptr)
		{
			printf("can not open file = %s\n", std::string(MEMORY_LOG_FILE_NAME).c_str());
		}
		else
		{
			PrintEntry printText;
			printText.m_preText = "new, delete list:";
			printText.m_isfPrintf = true;
			std::string sss = printText.printStr();
			fprintf(m_logfile, printText.printStr().c_str());
			fflush(m_logfile);
		}
#endif
		MemoryReady::instance().init();
	}

	~MemoryTestTrace()
	{
		MemoryReady::instance().uninit();
		dump(0, true);
#if defined(MEMORY_LOG_FILE)
		if (m_logfile != nullptr)
		{
			fclose(m_logfile);
		}
#endif
	}

public:
	static MemoryTestTrace& instance()
	{
		static MemoryTestTrace s_debugNewTracer;
		return s_debugNewTracer;
	}

public:
	void add(void* p, const char* file, int line, size_t size)
	{
		MemoryUniqueLock lock(m_mutex);
		if (m_mutex.lockCount() >= 2)
		{
			return;
		}
		const char* fileName = file;
		if (file != nullptr)
		{
			std::string strFile = file;
			int32_t left = (int32_t)strFile.find_last_of("/\\");
			fileName = (left == -1 ? file : &file[left + 1]);
		}
		m_totalSize += size;
		m_entry[p] = MemoryEntry(ShowTraceStack(), fileName, line, size);
#if defined(MEMORY_LOG_FILE) && defined(MEMORY_LIST)
		if (m_logfile != nullptr)
		{
			PrintEntry printText;
			printText.m_preText = "new";
			printText.m_p = p;
			printText.m_file = fileName;
			printText.m_line = line;
			printText.m_size = size;
			printText.m_totalSize = m_totalSize;
			printText.m_isfPrintf = true;
			fprintf(m_logfile, printText.printStr().c_str(), p, fileName, line, size, m_totalSize);
			fflush(m_logfile);
		}
#endif
	}

	void add(void* p, size_t size)
	{
		MemoryUniqueLock lock(m_mutex);
		if (m_mutex.lockCount() >= 2)
		{
			return;
		}
		const char* file = "third library file";
		m_totalSize += size;
		m_entry[p] = MemoryEntry(ShowTraceStack(), file, 0, size);
#if defined(MEMORY_LOG_FILE) && defined(MEMORY_LIST)
		if (m_logfile != nullptr)
		{
			PrintEntry printText;
			printText.m_preText = "new";
			printText.m_p = p;
			printText.m_file = file;
			printText.m_line = 0;
			printText.m_size = size;
			printText.m_totalSize = m_totalSize;
			printText.m_isfPrintf = true;
			fprintf(m_logfile, printText.printStr().c_str(), p, file, 0, size, m_totalSize);
			fflush(m_logfile);
		}
#endif
	}

	void remove(void* p)
	{
		MemoryUniqueLock lock(m_mutex);
		if (m_mutex.lockCount() >= 2)
		{
			return;
		}
		auto it = m_entry.find(p);
		if (it != m_entry.end())
		{
			size_t size = (*it).second.size();
			m_totalSize -= size;
#if defined(MEMORY_LOG_FILE) && defined(MEMORY_LIST)
			if (m_logfile != nullptr)
			{
				PrintEntry printText;
				printText.m_preText = "delete";
				printText.m_p = p;
				printText.m_size = size;
				printText.m_totalSize = m_totalSize;
				printText.m_isfPrintf = true;
				fprintf(m_logfile, printText.printStr().c_str(), p, size, m_totalSize);
				fflush(m_logfile);
			}
#endif
			m_entry.erase(it);
		}
		else
		{
#if defined(MEMORY_LOG_FILE) && defined(MEMORY_LIST)
			if (m_logfile != nullptr)
			{
				PrintEntry printText;
				printText.m_preText = "delete";
				printText.m_p = p;
				printText.m_nextText = "error point";
				printText.m_isfPrintf = true;
				fprintf(m_logfile, printText.printStr().c_str(), p);
				fflush(m_logfile);
			}
#endif
		}
	}

	void memoryPicture(int id)
	{
		dump(id, false);
	}

	void memoryStackPicture(int id)
	{
		dump(id, true);
	}

	void dump(int id, bool isPrintStack)
	{
		MemoryUniqueLock lock(m_mutex);
		if (!m_entry.empty())
		{
			PrintEntry printText;
			printText.m_preText = ("id = " + std::to_string(id) + ", memory leaks detected");
			printText.m_entrySize = m_entry.size();
			printText.m_isPrintf = true;
			printf(printText.printStr().c_str(), m_entry.size());
			printText.m_isPrintf = false;
			printText.m_isRCSend = true;
			RCSend(printText.printStr().c_str(), m_entry.size());
#if defined(MEMORY_LOG_FILE)
			if (m_logfile != nullptr)
			{
				fprintf(m_logfile, "\n");
				printText.m_isRCSend = false;
				printText.m_isfPrintf = true;
				fprintf(m_logfile, printText.printStr().c_str(), m_entry.size());
				fflush(m_logfile);
			}
#endif
			std::map<void*, MemoryEntry> entryMap;
			for (auto it = m_entry.begin(); it != m_entry.end(); ++it)
			{
				entryMap[it->first] = it->second;
			}

			for (auto it = entryMap.begin(); it != entryMap.end(); ++it)
			{
				void* p = it->first;
				std::string stack = it->second.stack();
				const char* file = it->second.file();
				int line = it->second.line();
				size_t size = it->second.size();

				PrintEntry printText;
				printText.m_p = p;
				printText.m_file = file;
				printText.m_line = line;
				printText.m_size = size;
				printText.m_isPrintf = true;
				printf(printText.printStr().c_str(), p, file, line, size);
				printText.m_isPrintf = false;
				printText.m_isRCSend = true;
				RCSend(printText.printStr().c_str(), p, file, line, size);
#if defined(MEMORY_LOG_FILE)
				if (m_logfile != nullptr)
				{
					if (isPrintStack)
					{
						printText.m_stack = stack;
					}
					printText.m_isRCSend = false;
					printText.m_isfPrintf = true;
					if (isPrintStack)
					{
						fprintf(m_logfile, printText.printStr().c_str(), p, file, line, size, stack.c_str());
					}
					else
					{
						fprintf(m_logfile, printText.printStr().c_str(), p, file, line, size);
					}
					fflush(m_logfile);
				}
#endif
			}
		}
		else
		{
			PrintEntry printText;
			printText.m_preText = ("id = " + std::to_string(id) + ", no leaks detected");
			printText.m_isPrintf = true;
			printf("%s", printText.printStr().c_str());
			printText.m_isPrintf = false;
			printText.m_isRCSend = true;
			RCSend("%s", printText.printStr().c_str());
#if defined(MEMORY_LOG_FILE)
			if (m_logfile != nullptr)
			{
				printText.m_isRCSend = false;
				printText.m_isfPrintf = true;
				fprintf(m_logfile, "%s", printText.printStr().c_str());
				fflush(m_logfile);
			}
#endif
		}
	}

	std::string ShowTraceStack()
	{
#ifdef _MSC_VER
		const int MAX_STACK_FRAMES = 12;
		void* pStack[MAX_STACK_FRAMES] = {};
		const int STACK_INFO_LEN = 1024;
		char szStackInfo[STACK_INFO_LEN * MAX_STACK_FRAMES] = {};
		char szFrameInfo[STACK_INFO_LEN] = {};

		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);
		WORD frames = CaptureStackBackTrace(0, MAX_STACK_FRAMES, pStack, NULL);

		_snprintf(szStackInfo, sizeof(szStackInfo), "stack traceback, threadId = %u\n", (uint32_t)::GetCurrentThreadId());

		for (WORD i = 0; i < frames; ++i)
		{
			DWORD64 address = (DWORD64)(pStack[i]);

			DWORD64 displacementSym = 0;
			char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
			PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;
			pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
			pSymbol->MaxNameLen = MAX_SYM_NAME;

			DWORD displacementLine = 0;
			IMAGEHLP_LINE64 line;
			line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

			if (SymFromAddr(process, address, &displacementSym, pSymbol) &&
				SymGetLineFromAddr64(process, address, &displacementLine, &line))
			{
				_snprintf(szFrameInfo, sizeof(szFrameInfo), "\t%s() at %s : %d (0x%I64u)\n",
					pSymbol->Name, line.FileName, line.LineNumber, pSymbol->Address);
			}
			else
			{
				DWORD lastError = GetLastError();
				if (lastError == 487)
				{
					_snprintf(szFrameInfo, sizeof(szFrameInfo), "\tcannot find stack, visit invalid address\n");
				}
				else
				{
					_snprintf(szFrameInfo, sizeof(szFrameInfo), "\tcannot find stack, last error = %u\n", lastError);
				}
			}
			strcat(szStackInfo, szFrameInfo);
		}
		return szStackInfo;
#elif __unix__
		std::thread::id threadId = std::this_thread::get_id();
		std::string result = "stack traceback, threadId = " + std::to_string((uint32_t)(*(__gthread_t*)(char*)(&threadId))) + "\n";
		void* buffer[1024] = {};
		int32_t nptrs = backtrace(buffer, 1024);
		char** strings = backtrace_symbols(buffer, nptrs);
		if (strings == nullptr)
		{
			result = "memory use up\n";
			return result;
		}
		int32_t index = -1;
		while (index++ != nptrs - 1)
		{
			std::string stack = strings[index];
			result += ("[" + std::to_string(index) + "] " + stack + "\n");
		}
		free(strings);
#endif
	}

	std::string GetCurrentExePath()
	{
		char szFilePath[1024] = {};
#ifdef _WIN32
		::GetModuleFileNameA(NULL, szFilePath, 1024);
#elif __unix__
		::readlink("/proc/self/exe", szFilePath, 1024);
#endif
		return GetName(szFilePath, 4);
	}

	std::string GetCurrentExeName()
	{
		char szFilePath[1024] = {};
#ifdef _WIN32
		::GetModuleFileNameA(NULL, szFilePath, 1024);
#elif __unix__
		::readlink("/proc/self/exe", szFilePath, 1024);
#endif
		return GetName(szFilePath, 3);
	}

	std::string GetName(const std::string& path, int32_t flag)
	{
		int32_t left = (int32_t)path.find_last_of("/\\");
		std::string name = path.substr(left + 1, path.length() - left - 1);
		int32_t point = (int32_t)name.find_last_of(".");
		switch (flag)
		{
		case 1:
		{
			return name;
		}
		case 2:
		{
			return name.substr(point + 1, point == -1 ? 0 : name.length() - point - 1);
		}
		case 3:
		{
			return name.substr(0, point == -1 ? name.length() : point);
		}
		case 4:
		{
			return path.substr(0, left + 1);
		}
		default:
			return "";
		}
	}
	
private:
	std::unordered_map<void*, MemoryEntry> m_entry;
	MemoryRecursiveMutex m_mutex;
	size_t m_totalSize;
#if defined(MEMORY_LOG_FILE)
	FILE* m_logfile;
#endif
};

class MemoryGlobal
{
public:
	MemoryGlobal()
	{
		MemoryTestTrace::instance();
	}
};

const MemoryGlobal g_memoryGlobal;

inline void* operator new(size_t size, const char* file, int line)
{
	void* p = malloc(size);
	if (MemoryReady::instance().isReady())
	{
		MemoryTestTrace::instance().add(p, file, line, size);
	}
	return p;
}

inline void* operator new[](size_t size, const char* file, int line)
{
	void* p = malloc(size);
	if (MemoryReady::instance().isReady())
	{
		MemoryTestTrace::instance().add(p, file, line, size);
	}
	return p;
}

inline void operator delete(void* p, const char* file, int line)
{
#ifdef _MSC_VER
	printf("into operator delete p = 0x%p, file = %s, line = %d\n", p, file, line);
#elif __unix__
	printf("into operator delete p = %p, file = %s, line = %d\n", p, file, line);
#endif
	file = nullptr;
	line = 0;
	if (MemoryReady::instance().isReady())
	{
		MemoryTestTrace::instance().remove(p);
	}
	free(p);
}

inline void operator delete[](void* p, const char* file, int line)
{
#ifdef _MSC_VER
	printf("into operator delete[] p = 0x%p, file = %s, line = %d\n", p, file, line);
#elif __unix__
	printf("into operator delete[] p = %p, file = %s, line = %d\n", p, file, line);
#endif
	
	file = nullptr;
	line = 0;
	if (MemoryReady::instance().isReady())
	{
		MemoryTestTrace::instance().remove(p);
	}
	free(p);
}

inline void* operator new(size_t size)
{
	void* p = malloc(size);
	if (MemoryReady::instance().isReady())
	{
		MemoryTestTrace::instance().add(p, size);
	}
	return p;
}

inline void* operator new[](size_t size)
{
	void* p = malloc(size);
	if (MemoryReady::instance().isReady())
	{
		MemoryTestTrace::instance().add(p, size);
	}
	return p;
}

inline void operator delete(void* p)
{
	if (MemoryReady::instance().isReady())
	{
		MemoryTestTrace::instance().remove(p);
	}
	free(p);
}

inline void operator delete[](void* p)
{
	if (MemoryReady::instance().isReady())
	{
		MemoryTestTrace::instance().remove(p);
	}
	free(p);
}

inline void* DebugMalloc(size_t size, const char* file, int line)
{
	void* p = ::malloc(size);
	if (MemoryReady::instance().isReady())
	{
		MemoryTestTrace::instance().add(p, file, line, size);
	}
	return p;
}

inline void* DebugCalloc(size_t count, size_t size, const char* file, int line)
{
	void* p = ::calloc(count, size);
	if (MemoryReady::instance().isReady())
	{
		MemoryTestTrace::instance().add(p, file, line, count * size);
	}
	return p;
}

inline void* DebugRealloc(void* memory, size_t newSize, const char* file, int line)
{
	void* p = ::realloc(memory, newSize);
	if (MemoryReady::instance().isReady())
	{
		MemoryTestTrace::instance().remove(memory);
		MemoryTestTrace::instance().add(p, file, line, newSize);
	}
	return p;
}

#define new new(__FILE__, __LINE__)
#define malloc(size) DebugMalloc(size, __FILE__, __LINE__)
#define calloc(count, size) DebugCalloc(count, size, __FILE__, __LINE__)
#define realloc(memory, newSize) DebugRealloc(memory, newSize, __FILE__, __LINE__)

#endif // __MEMORY_TEST_H__