#include "SharedMemory.h"
#include <Windows.h>

SharedMemory::SharedMemory(const std::string& name, uint32_t size):
m_memoryHandle(nullptr),
m_memoryPtr(nullptr),
m_memoryName(name),
m_processReadWriteMutex((name + "_lock").c_str())
{
	if (size != 0)
	{
		m_memoryHandle = ::CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, name.empty() ? nullptr : name.c_str());
	}
}

SharedMemory::~SharedMemory()
{
	close();
	if (m_memoryHandle != nullptr)
	{
		::CloseHandle(m_memoryHandle);
		m_memoryHandle = nullptr;
	}
}

uint32_t SharedMemory::size()
{
	open(true);
	if (m_memoryHandle == nullptr)
	{
		return 0;
	}
	MEMORY_BASIC_INFORMATION mem_info;
	::VirtualQuery(::MapViewOfFile(m_memoryHandle, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0), &mem_info, sizeof(mem_info));
	return mem_info.RegionSize;
}

void* SharedMemory::memory()
{
	return m_memoryPtr;
}

void SharedMemory::read()
{
	m_processReadWriteMutex.read();
	open(true);
	if (m_memoryHandle == nullptr)
	{
		return;
	}
	m_memoryPtr = ::MapViewOfFile(m_memoryHandle, FILE_MAP_READ, 0, 0, 0);
}

void SharedMemory::unread()
{
	m_processReadWriteMutex.unread();
}

void SharedMemory::write()
{
	m_processReadWriteMutex.write();
	open(false);
	if (m_memoryHandle == nullptr)
	{
		return;
	}
	m_memoryPtr = ::MapViewOfFile(m_memoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
}

void SharedMemory::unwrite()
{
	m_processReadWriteMutex.unwrite();
}

bool SharedMemory::trywrite()
{
	if (!m_processReadWriteMutex.trywrite())
	{
		return false;
	}
	open(false);
	if (m_memoryHandle == nullptr)
	{
		m_processReadWriteMutex.unwrite();
		return false;
	}
	m_memoryPtr = ::MapViewOfFile(m_memoryHandle, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	return true;
}

void SharedMemory::close()
{
	if (m_memoryPtr)
	{
		::UnmapViewOfFile(m_memoryPtr);
		m_memoryPtr = nullptr;
	}
}

void SharedMemory::open(bool bReadOnly)
{
	m_memoryHandle = ::OpenFileMapping(bReadOnly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS, FALSE, m_memoryName.empty() ? NULL : m_memoryName.c_str());
}

////Ð´
//int32_t main()
//{
//	SharedMemory mem("mmsa", 4095);
//	int32_t size = mem.size();
//	{
//		WriteLock writelock(mem);
//		void* ptr = mem.memory();
//		memcpy(ptr, "121212\0", 7);
//	}
//	getchar();
//	return 0;
//}
//
////¶Á
//int32_t main()
//{
//	SharedMemory mem("mmsa");
//	int32_t size = mem.size();
//	{
//		ReadLock readlock(mem);
//		void* ptr = mem.memory();
//		printf("%s\n", ptr);
//	}
//	getchar();
//	return 0;
//}