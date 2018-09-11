#pragma once
#include "SharedMemoryMacro.h"
#include <stdint.h>
#include <string>
#include "ReadWriteMutex/ReadWriteMutexAPI.h"

class SharedMemoryAPI SharedMemory : public ReadWriteMutexBase
{
public:
	SharedMemory(const std::string& name, uint32_t size = 0);
	~SharedMemory();

public:
	uint32_t size();
	void* memory();
	virtual void read();
	virtual void unread();
	virtual void write();
	virtual void unwrite();
	void* writeWithoutLock();
	bool trywrite();
	void close();

protected:
	void open(bool bReadOnly);

private:
	SharedMemory(const SharedMemory& other);
	SharedMemory& operator = (const SharedMemory& other);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_memoryName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	HANDLE m_memoryHandle;
	void* m_memoryPtr;
	ProcessReadWriteMutex m_processReadWriteMutex;
};