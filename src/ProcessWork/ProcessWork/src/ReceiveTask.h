#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessReceiveCallback;
class SharedMemory;

/** ��������
*/
class ReceiveTask : public CTask
{
public:
	/** ���캯��
	*/
	ReceiveTask();

public:
	/** ִ������
	*/
	void DoTask();

	/** ���ò���
	@param [in] assign ��ȡ�Ļ�������
	@param [in] callback ���ջص���
	@param [in] memoryMap �����ڴ���
	*/
	void setParam(int32_t assign,
		ProcessReceiveCallback* callback,
		std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* memoryMap);

private:
	int32_t m_assign;
	ProcessReceiveCallback* m_callback;
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* m_memoryMap;
};