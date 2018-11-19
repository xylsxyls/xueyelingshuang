#pragma once
#include "SharedMemoryMacro.h"
#include <stdint.h>
#include <string>
#include "ReadWriteMutex/ReadWriteMutexAPI.h"

class SharedMemoryAPI SharedMemory : public ReadWriteMutexBase
{
public:
	/** 构造函数
	@param [in] name 共享内存名
	@param [in] size 共享内存大小，如果是0表示打开
	*/
	SharedMemory(const std::string& name, uint32_t size = 0);
	~SharedMemory();

public:
	uint32_t size();
	std::string mapName();
	void* memory();
	virtual void read();
	virtual void unread();
	virtual void write();
	virtual void unwrite();
	void* readWithoutLock();
	void* writeWithoutLock();
	bool trywrite();
	void close();

public:
	static std::string mapName(HANDLE memoryHandle, int32_t bufferSize = 1024);

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
	void* m_readMemoryPtr;
	void* m_writeMemoryPtr;
	ProcessReadWriteMutex m_processReadWriteMutex;
};