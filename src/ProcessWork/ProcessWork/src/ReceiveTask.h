#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <vector>
#include "LockFreeQueue/LockFreeQueueAPI.h"

class ProcessReceiveCallback;
class Semaphore;

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

	/** �ж�����
	*/
	void StopTask();

	/** ���ò���
	@param [in] callback ���ջص���
	@param [in] receiveQueue ���ն���
	@param [in] receiveSemaphore �����ź���
	*/
	void setParam(std::vector<ProcessReceiveCallback*>* callback,
		LockFreeQueue<char*>* receiveQueue,
		Semaphore* receiveSemaphore);

private:
	std::vector<ProcessReceiveCallback*>* m_callback;
	LockFreeQueue<char*>* m_receiveQueue;
	Semaphore* m_receiveSemaphore;
	std::atomic<bool> m_exit;
};