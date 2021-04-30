#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessReceiveCallback;
class SharedMemory;

/** ��������
*/
class CopyTask : public CTask
{
public:
	/** ���캯��
	*/
	CopyTask();

public:
	/** ִ������
	*/
	void DoTask();

	/** ���ò���
	@param [in] assign ��ȡ�Ļ�������
	@param [in] callback ���ջص���
	@param [in] memoryMap �����ڴ���
	@param [in] receiveThread �����߳�
	*/
	void setParam(int32_t assign,
		std::vector<ProcessReceiveCallback*>* callback,
		std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* memoryMap,
		const std::shared_ptr<CTaskThread>& receiveThread);

private:
	int32_t m_assign;
	std::vector<ProcessReceiveCallback*>* m_callback;
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* m_memoryMap;
	std::shared_ptr<CTaskThread> m_receiveThread;
};