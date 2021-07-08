#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

class Semaphore;
/** ��������
*/
class SendTask : public CTask
{
public:
	/** ���캯��
	*/
	SendTask();

public:
	/** ִ������
	*/
	void DoTask();

	/** �ж�����
	*/
	void StopTask();

	/** ���ò���
	@param [in] waitEndPost ���ͽ������Ƿ��˳�ѭ��
	@param [in] postQueue �������ݶ���
	@param [in] postSemaphore �����ź���
	*/
	void setParam(std::atomic<bool>* waitEndPost, LockFreeQueue<char*>* postQueue, Semaphore* postSemaphore);

private:
	std::atomic<bool>* m_waitEndPost;
	LockFreeQueue<char*>* m_postQueue;
	Semaphore* m_postSemaphore;
	std::atomic<bool> m_exit;
};