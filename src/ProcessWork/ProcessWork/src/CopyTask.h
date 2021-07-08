#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

class Semaphore;
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

	/** �ж�����
	*/
	void StopTask();

	/** ���ò���
	@param [in] areaAssign �������ŷ������
	@param [in] memoryMap �����ڴ���
	@param [in] assignQueue ����Ŷ���
	@param [in] assignSemaphore ������ź���
	@param [in] receiveQueue ���ն���
	@param [in] receiveSemaphore �����ź���
	*/
	void setParam(SharedMemory* areaAssign,
		std::map<int32_t, std::shared_ptr<SharedMemory>>* memoryMap,
		LockFreeQueue<int32_t>* assignQueue,
		Semaphore* assignSemaphore,
		LockFreeQueue<char*>* receiveQueue,
		Semaphore* receiveSemaphore);

private:
	SharedMemory* m_areaAssign;
	std::map<int32_t, std::shared_ptr<SharedMemory>>* m_memoryMap;
	LockFreeQueue<int32_t>* m_assignQueue;
	Semaphore* m_assignSemaphore;
	LockFreeQueue<char*>* m_receiveQueue;
	Semaphore* m_receiveSemaphore;
	std::atomic<bool> m_exit;
};