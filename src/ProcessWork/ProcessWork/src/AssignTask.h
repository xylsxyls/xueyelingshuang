#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class Semaphore;
class SharedMemory;
/** ��������
*/
class AssignTask : public CTask
{
public:
	/** ���캯��
	*/
	AssignTask();

public:
	/** ִ������
	*/
	void DoTask();

	/** ֹͣ����
	*/
	void StopTask();

	/** ���ò���
	@param [in] assignSemaphore �����ź�
	@param [in] assignEndSemaphore ��������ź�
	@param [in] areaAssign �������Ż������
	*/
	void setParam(Semaphore* assignSemaphore,
		Semaphore* assignEndSemaphore,
		SharedMemory* areaAssign);

private:
	Semaphore* m_assignSemaphore;
	Semaphore* m_assignEndSemaphore;
	SharedMemory* m_areaAssign;
	std::atomic<bool> m_exit;
};