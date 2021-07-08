#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

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
	@param [in] readSemaphore ��ȡ�ź�
	@param [in] areaCount ����������
	@param [in] areaRead �������Ŷ�ȡ�������
	@param [in] assignQueue ����Ŷ���
	@param [in] assignSemaphore ������ź���
	*/
	void setParam(Semaphore* readSemaphore,
		int32_t areaCount,
		SharedMemory* areaRead,
		LockFreeQueue<int32_t>* assignQueue,
		Semaphore* assignSemaphore);

private:
	Semaphore* m_readSemaphore;
	int32_t m_areaCount;
	SharedMemory* m_areaRead;
	LockFreeQueue<int32_t>* m_assignQueue;
	Semaphore* m_assignSemaphore;
	std::atomic<bool> m_exit;
};