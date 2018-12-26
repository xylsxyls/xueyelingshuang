#pragma once
#include "ProcessWorkMacro.h"
#include <stdint.h>
#include <map>

class ReceiveCallback;
class SharedMemory;
class ProcessReadWriteMutex;

class ProcessWorkAPI ProcessWork
{
	friend class ReceiveTask;
	friend class WorkTask;
	friend class DeleteTask;
	friend class CreateKeyTask;
	friend class CreateDataTask;
public:
	static ProcessWork& instance();

protected:
	ProcessWork();

public:
	void initReceive(ReceiveCallback* callback);

	/** 向服务端发送字符串，有序收到单线程，无序收到多线程
	@param [in] buffer 字符串地址
	@param [in] length 长度
	@param [in] pid 目标进程id
	@param [in] protocolId 协议号
	*/
	void send(const char* buffer, int32_t length, int32_t pid, int32_t protocolId = 1);

	/** 向服务端发送字符串，有序收到单线程，无序收到多线程
	@param [in] buffer 字符串地址
	@param [in] length 长度
	@param [in] processName 目标进程名
	@param [in] protocolId 协议号
	*/
	void send(const char* buffer, int32_t length, const std::string processName, int32_t protocolId = 1);

	/** 退出线程
	*/
	void uninit();

private:
	SharedMemory* m_position;
	SharedMemory* m_data;
	SharedMemory* m_key;
	SharedMemory* m_readData;
	SharedMemory* m_readKey;
	ProcessReadWriteMutex* m_positionMutex;
	uint32_t m_receiveThreadId;
	uint32_t m_workThreadId;
	uint32_t m_deleteThreadId;
	uint32_t m_deleteKeyThreadId;
	uint32_t m_deleteDataThreadId;
	uint32_t m_createKeyThreadId;
	uint32_t m_createDataThreadId;
	int32_t m_processPid;

	ReceiveCallback* m_callback;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<int32_t, SharedMemory*> m_dataMemoryMap;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};