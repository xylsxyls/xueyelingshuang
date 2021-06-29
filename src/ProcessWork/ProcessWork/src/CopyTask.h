#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <vector>

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
	@param [in] areaAssign �������ŷ��仺�����
	@param [in] memoryMap �����ڴ���
	@param [in] receiveThread �����߳�
	*/
	void setParam(int32_t assign,
		std::vector<ProcessReceiveCallback*>* callback,
		SharedMemory* areaAssign,
		std::map<int32_t, std::shared_ptr<SharedMemory>>* memoryMap,
		const std::shared_ptr<CTaskThread>& receiveThread);

private:
	int32_t m_assign;
	std::vector<ProcessReceiveCallback*>* m_callback;
	SharedMemory* m_areaAssign;
	std::map<int32_t, std::shared_ptr<SharedMemory>>* m_memoryMap;
	std::shared_ptr<CTaskThread> m_receiveThread;
};