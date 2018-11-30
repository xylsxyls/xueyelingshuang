#pragma once
#include "ProcessClientMacro.h"
#include <stdint.h>
#include <map>

class ReceiveCallback;
class SharedMemory;
class ProcessReadWriteMutex;

class ProcessClientAPI ProcessClient
{
	friend class ReceiveTask;
	friend class WorkTask;
	friend class DeleteTask;
public:
	static ProcessClient& instance();

protected:
	ProcessClient();

public:
	void initReceive(ReceiveCallback* callback);

	/** 向服务端发送字符串，有序收到单线程，无序收到多线程
	@param [in] buffer 字符串地址
	@param [in] length 长度
	@param [in] pid 目标进程id
	@param [in] isOrdered 是否有序
	@param [in] protocolId 协议号
	*/
	void send(char* buffer, int32_t length, int32_t pid, bool isOrdered = true, int32_t protocolId = 1);

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
	int32_t m_sendPid;

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