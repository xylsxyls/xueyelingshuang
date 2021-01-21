#pragma once
#include "SharedMemoryMacro.h"
#include <stdint.h>
#include <string>

typedef void* HANDLE;

class SharedMemoryAPI SharedMemory
{
public:
	/** 构造函数
	@param [in] name 共享内存名
	@param [in] size 共享内存大小，如果是0表示打开
	*/
	SharedMemory(const std::string& name, uint32_t size = 0);

	/** 析构函数
	*/
	~SharedMemory();

private:
	/** 拷贝构造函数
	*/
	SharedMemory(const SharedMemory& other);

	/** 赋值函数
	*/
	SharedMemory operator = (const SharedMemory& other);

public:
	/** 创建大小
	*/
	uint32_t size();

	/** 真实大小
	*/
	uint32_t realSize();

	/** 共享内存名
	*/
	std::string mapName();

	/** 共享内存地址
	*/
	void* memory();

	/** 共享内存读取地址
	*/
	void* readWithoutLock();

	/** 共享内存写入地址
	*/
	void* writeWithoutLock();

	/** 关闭共享内存
	*/
	void close();

	/** 清空共享内存
	*/
	void clear();

public:
#ifdef _MSC_VER
	static std::string mapName(HANDLE memoryHandle, int32_t bufferSize = 1024);
#endif

protected:
	void open(bool bReadOnly);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_memoryName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef _MSC_VER
	HANDLE m_memoryHandle;
	void* m_readMemoryPtr;
	void* m_writeMemoryPtr;
#elif __unix__
	int m_shmid;
#endif
	void* m_memoryPtr;
	uint32_t m_size;
};