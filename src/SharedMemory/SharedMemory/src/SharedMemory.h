#pragma once
#include "SharedMemoryMacro.h"
#include <stdint.h>
#include <string>

typedef void* HANDLE;

class SharedMemoryAPI SharedMemory
{
public:
	/** ���캯��
	@param [in] name �����ڴ���
	@param [in] size �����ڴ��С�������0��ʾ��
	*/
	SharedMemory(const std::string& name, uint32_t size = 0);
	~SharedMemory();

public:
	uint32_t size();
	std::string mapName();
	void* memory();
	void* readWithoutLock();
	void* writeWithoutLock();
	void close();
	void clear();

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
	uint32_t m_size;
};