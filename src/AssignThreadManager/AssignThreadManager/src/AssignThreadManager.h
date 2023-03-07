#pragma once
#include "AssignThreadManagerMacro.h"
#include <stdint.h>
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

/** ���˷ַ�������̹߳�����
*/
class AssignThreadManagerAPI AssignThreadManager
{
protected:
	/** ���캯��
	*/
	AssignThreadManager();

public:
	/** ��һʵ��
	@return ���ص�һʵ��
	*/
	static AssignThreadManager& instance();

public:
	/** ��ʼ��
	@param [in] �ַ��߸���
	*/
	void init(int32_t assignCount);

	/** ������̴߳���ַ�����
	@param [in] vecAssignTask �ַ�����
	*/
	void postAssignTask(const std::vector<std::shared_ptr<CTask>>& vecAssignTask);
	
	/** ��ִ���̴߳���ִ������
	@param [in] spTask ִ������
	@param [in] taskLevel �������ȼ�
	*/
	void postTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel = 1);

	/** ֹͣ��������
	*/
	void stopAllTask();

	/** ����ʼ��
	*/
	void uninit();

protected:
	//����߳�
	uint32_t m_enterThreadId;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//�����̼߳���
	std::vector<uint32_t> m_vecAssignThreadId;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	//ִ���߳�
	uint32_t m_threadId;
};