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

	/** �����˷����ַ����������յ����̣߳������յ����߳�
	@param [in] buffer �ַ�����ַ
	@param [in] length ����
	@param [in] pid Ŀ�����id
	@param [in] isOrdered �Ƿ�����
	@param [in] protocolId Э���
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