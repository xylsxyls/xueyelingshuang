#pragma once
#include "CTaskThreadManagerMacro.h"
#include <stdint.h>

class Semaphore;

//���Ҫ�������ִ�н���������ڴ���task��ʱ��Ͱѻ�ý����ָ�봫��task�ڲ����̰߳�ȫ�����ⲿά��
/* �߳�������
*/
class CTaskThreadManagerAPI CTask
{
	friend class CTaskThread;
public:
	//��Ҫ�ڹ��������������ʱ�������ή�ͷ��������Ч�ʣ��������ڴ�й¶�����
	/* ���캯��
	*/
	CTask();
	
	/* ���캯��
	@param [in] taskId ����ID����ID�Ų���ΨһID����˼��һ���������ֹͣһ����������
	*/
	CTask(int32_t taskId);

	/* ��������
	*/
	virtual ~CTask();

public:
	/* �߳���ִ�е�������
	*/
	virtual void DoTask() = 0;
	
	/* �ж�����
	*/
	virtual void StopTask();
	
	/* �����񱻸������ȼ������񶥵����Ƿ�����
	@return �����Ƿ�����
	*/
	virtual bool ReExecute();
	
	/* ���������ͨ���˺�������һ��������
	@return ��������ָ��
	*/
	virtual CTask* Clone();

	//��ID�Ų���ΨһID����˼��һ���������ֹͣһ����������
	/* ��ȡ����ID
	@return ��������ID
	*/
	int32_t GetTaskId();

private:
	void SetWaitForSendHandle(Semaphore* waitForSend);
	
	Semaphore* GetWaitForSendHandle();

protected:
	//����ID
	int32_t m_taskId;

	//SendTask���õ��������¼�
	Semaphore* m_waitForSend;
};
