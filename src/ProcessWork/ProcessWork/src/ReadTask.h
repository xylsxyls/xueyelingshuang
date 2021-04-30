#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <vector>

class ProcessReceiveCallback;
class Semaphore;
class SharedMemory;

/** ��������
*/
class ReadTask : public CTask
{
public:
	/** ���캯��
	*/
	ReadTask();

public:
	/** ִ������
	*/
	void DoTask();

	/** ֹͣ����
	*/
	void StopTask();

	/** ���ò���
	@param [in] callback ���ջص���
	@param [in] readSemaphore ��ȡ�ź�
	@param [in] readEndSemaphore ��ȡ����ź�
	@param [in] area �������Ż���
	@param [in] memoryMap �����ڴ���
	@param [in] receiveThread �����߳�
	*/
	void setParam(std::vector<ProcessReceiveCallback*>* callback,
		Semaphore* readSemaphore,
		Semaphore* readEndSemaphore,
		SharedMemory* area,
		std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* memoryMap,
		const std::shared_ptr<CTaskThread>& copyThread,
		const std::shared_ptr<CTaskThread>& receiveThread);

private:
	std::atomic<bool> m_exit;
	std::vector<ProcessReceiveCallback*>* m_callback;
	Semaphore* m_readSemaphore;
	Semaphore* m_readEndSemaphore;
	SharedMemory* m_area;
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* m_memoryMap;
	std::shared_ptr<CTaskThread> m_copyThread;
	std::shared_ptr<CTaskThread> m_receiveThread;
};