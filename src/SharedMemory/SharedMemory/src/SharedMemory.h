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

	/** ��������
	*/
	~SharedMemory();

private:
	/** �������캯��
	*/
	SharedMemory(const SharedMemory& other);

	/** ��ֵ����
	*/
	SharedMemory operator = (const SharedMemory& other);

public:
	/** ������С
	*/
	uint32_t size();

	/** ��ʵ��С
	*/
	uint32_t realSize();

	/** �����ڴ���
	*/
	std::string mapName();

	/** �����ڴ��ַ
	*/
	void* memory();

	/** �����ڴ��ȡ��ַ
	*/
	void* readWithoutLock();

	/** �����ڴ�д���ַ
	*/
	void* writeWithoutLock();

	/** �رչ����ڴ�
	*/
	void close();

	/** ��չ����ڴ�
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