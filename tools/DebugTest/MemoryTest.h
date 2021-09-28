#ifndef __DEBUGNEW_H__
#define __DEBUGNEW_H__

#include <map>
#include <atomic>

#ifdef _MSC_VER
#include <windows.h>
#include <dbghelp.h>
#include "D:\\SendToMessageTest.h"

#pragma comment(lib, "Dbghelp.lib")
#endif

#define LOG_FILE
#ifdef LOG_FILE
#define LOG_FILE_NAME "MemoryTest.log"
#endif

#define MEMORY_PICTURE MemoryTestTrace::instance().memoryPicture
#define MEMORY_STACK_PICTURE MemoryTestTrace::instance().memoryStackPicture

class MemoryRecursiveMutex
{
public:
	MemoryRecursiveMutex():
		m_lockCount(0)
	{
		InitializeCriticalSection(&m_lock);
	}

	~MemoryRecursiveMutex()
	{
		DeleteCriticalSection(&m_lock);
	}

	void lock()
	{
		EnterCriticalSection(&m_lock);
		++m_lockCount;
	}

	void unlock()
	{
		--m_lockCount;
		LeaveCriticalSection(&m_lock);
	}

	int lockCount()
	{
		return m_lockCount;
	}

private:
	int m_lockCount;
	CRITICAL_SECTION m_lock;
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
	{
#if defined(LOG_FILE)
		m_logfile = fopen((GetCurrentExePath() + GetCurrentExeName() + LOG_FILE_NAME).c_str(), "wt");
		if (m_logfile == nullptr)
		{
			printf(" Error! file: debugnew.log can not open@!\n");
		}
		else
		{
			fprintf(m_logfile, "new, delete list:\n");
			fflush(m_logfile);
		}
#endif
		MemoryReady::instance().init();
	}

	~MemoryTestTrace()
	{
		MemoryReady::instance().uninit();
		dump(false);
#if defined(LOG_FILE)
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
		void* sss = this;
		MemoryUniqueLock lock(m_mutex);
		if (m_mutex.lockCount() >= 2)
		{
			return;
		}
		file = strrchr(file, '//') == NULL ? file : strrchr(file, '//') + 1;
		m_totalSize += size;
		m_entry[p] = MemoryEntry(ShowTraceStack(), file, line, size);
#if defined(LOG_FILE)
		if (m_logfile != nullptr)
		{
			fprintf(m_logfile, "*N p = 0x%p, size = %6d,  %-22s, Line: %4d.  totalsize = %8d\n", p, size, file, line, m_totalSize);
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
#if defined(LOG_FILE)
		if (m_logfile != nullptr)
		{
			fprintf(m_logfile, "*N p = 0x%p, size = %6d,  %-22s, Line: %4d.  totalsize = %8d\n", p, size, file, 0, m_totalSize);
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
#if defined(LOG_FILE)
			if (m_logfile != nullptr)
			{
				fprintf(m_logfile, "#D p = 0x%p, size = %6u.%39stotalsize = %8d\n", p, size, "-----------------------------------  ", m_totalSize);
				fflush(m_logfile);
			}
#endif
			m_entry.erase(it);
		}
		else
		{
#if defined(LOG_FILE)
			if (m_logfile != nullptr)
			{
				fprintf(m_logfile, "#D p = 0x%p. error point!!!\n", p);
				fflush(m_logfile);
			}
#endif
		}
	}

	void memoryPicture()
	{
		dump(false);
	}

	void memoryStackPicture()
	{
		dump(true);
	}

	void dump(bool isPrintStack)
	{
		void* sss = this;
		MemoryUniqueLock lock(m_mutex);
		if (!m_entry.empty())
		{
			printf("%d memory leaks detected\n", (int32_t)m_entry.size());
			RCSend("%d memory leaks detected", (int32_t)m_entry.size());
#if defined(LOG_FILE)
			if (m_logfile != nullptr)
			{
				fprintf(m_logfile, "\n\n***%d memory leaks detected\n", (int32_t)m_entry.size());
				fflush(m_logfile);
			}
#endif
			for (auto it = m_entry.begin(); it != m_entry.end(); ++it)
			{
				void* p = it->first;
				std::string stack = it->second.stack();
				const char* file = it->second.file();
				int line = it->second.line();
				size_t size = it->second.size();
				if (isPrintStack)
				{
#if defined _WIN64 || defined __x86_64__
					printf("0x%p, %s, %d, size = %I64d\n%s\n", p, file, line, size, stack.c_str());
					RCSend("0x%p, %s, %d, size = %I64d", p, file, line, size);
#if defined(LOG_FILE)
					if (m_logfile != nullptr)
					{
						fprintf(m_logfile, "0x%p, %s, %d, size = %I64d\n%s\n", p, file, line, size, stack.c_str());
						fflush(m_logfile);
					}
#endif
#else
					printf("0x%p, %s, %d, size = %d\n%s\n", p, file, line, size, stack.c_str());
					RCSend("0x%p, %s, %d, size = %d", p, file, line, size);
#if defined(LOG_FILE)
					if (m_logfile != nullptr)
					{
						fprintf(m_logfile, "0x%p, %s, %d, size = %d\n%s\n", p, file, line, size, stack.c_str());
						fflush(m_logfile);
					}
#endif
#endif // _WIN64
				}
				else
				{
#if defined _WIN64 || defined __x86_64__
					printf("0x%p, %s, %d, size = %I64d\n", p, file, line, size);
					RCSend("0x%p, %s, %d, size = %I64d", p, file, line, size);
#if defined(LOG_FILE)
					if (m_logfile != nullptr)
					{
						fprintf(m_logfile, "0x%p, %s, %d, size = %I64d\n", p, file, line, size);
						fflush(m_logfile);
					}
#endif
#else
					printf("0x%p, %s, %d, size = %d\n", p, file, line, size);
					RCSend("0x%p, %s, %d, size = %d", p, file, line, size);
#if defined(LOG_FILE)
					if (m_logfile != nullptr)
					{
						fprintf(m_logfile, "0x%p, %s, %d, size = %d\n", p, file, line, size);
						fflush(m_logfile);
					}
#endif
#endif // _WIN64
				}
			}
		}
		else
		{
			printf("no leaks detected\n");
			RCSend("no leaks detected");
#if defined(LOG_FILE)
			if (m_logfile != nullptr)
			{
				fprintf(m_logfile, "no leaks detected\n");
				fflush(m_logfile);
			}
#endif
		}
	}

	std::string ShowTraceStack()
	{
		const int MAX_STACK_FRAMES = 12;
		void* pStack[MAX_STACK_FRAMES] = {};
		const int STACK_INFO_LEN = 1024;
		char szStackInfo[STACK_INFO_LEN * MAX_STACK_FRAMES] = {};
		char szFrameInfo[STACK_INFO_LEN] = {};

		HANDLE process = GetCurrentProcess();
		SymInitialize(process, NULL, TRUE);
		WORD frames = CaptureStackBackTrace(0, MAX_STACK_FRAMES, pStack, NULL);

		_snprintf(szStackInfo, sizeof(szStackInfo), "stack traceback, threadId = %d\n", (int32_t)::GetCurrentThreadId());

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
				_snprintf(szFrameInfo, sizeof(szFrameInfo), "\t%s() at %s : %d (0x%p)\n",
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
					_snprintf(szFrameInfo, sizeof(szFrameInfo), "\tcannot find stack, last error = %d\n", lastError);
				}
			}
			strcat(szStackInfo, szFrameInfo);
		}
		return szStackInfo;
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
	std::map<void*, MemoryEntry> m_entry;
	MemoryRecursiveMutex m_mutex;
	size_t m_totalSize;
#if defined(LOG_FILE)
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
	printf("into operator delete p = %p, file = %s, line = %d\n", p, file, line);
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
	printf("into operator delete[] p = %p, file = %s, line = %d\n", p, file, line);
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

#endif // __DEBUGNEW_H__