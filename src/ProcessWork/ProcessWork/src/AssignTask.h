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
	@param [in] area �������Ż���
	@param [in] memoryMap �����ڴ���
	*/
	void setParam(Semaphore* assignSemaphore,
		Semaphore* assignEndSemaphore,
		SharedMemory* area,
		std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* memoryMap);

private:
	std::atomic<bool> m_exit;
	Semaphore* m_assignSemaphore;
	Semaphore* m_assignEndSemaphore;
	SharedMemory* m_area;
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* m_memoryMap;
};