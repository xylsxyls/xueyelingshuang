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
	@param [in] areaAssign �������ŷ��仺�����
	@param [in] areaRead �������Ŷ�ȡ�������
	@param [in] memoryMap ���л�����
	@param [in] copyThread �����߳�
	@param [in] receiveThread �����߳�
	*/
	void setParam(std::vector<ProcessReceiveCallback*>* callback,
		Semaphore* readSemaphore,
		SharedMemory* areaAssign,
		SharedMemory* areaRead,
		std::map<int32_t, std::shared_ptr<SharedMemory>>* memoryMap,
		const std::shared_ptr<CTaskThread>& copyThread,
		const std::shared_ptr<CTaskThread>& receiveThread);

private:
	std::atomic<bool> m_exit;
	std::vector<ProcessReceiveCallback*>* m_callback;
	Semaphore* m_readSemaphore;
	SharedMemory* m_areaAssign;
	SharedMemory* m_areaRead;
	std::map<int32_t, std::shared_ptr<SharedMemory>>* m_memoryMap;
	std::shared_ptr<CTaskThread> m_copyThread;
	std::shared_ptr<CTaskThread> m_receiveThread;
};