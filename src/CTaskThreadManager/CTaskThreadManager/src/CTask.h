#pragma once
#include "CTaskThreadManagerMacro.h"
#include <stdint.h>

class Semaphore;
//���Ҫ�������ִ�н���������ڴ���task��ʱ��Ͱѻ�ý����ָ�봫��task�ڲ����̰߳�ȫ�����ⲿά��
class CTaskThreadManagerAPI CTask
{
	friend class CTaskThread;
public:
	//��Ҫ�ڹ��������������ʱ�������ή�ͷ��������Ч�ʣ��������ڴ�й¶�����
	CTask();
    CTask(int32_t taskId);

	virtual ~CTask();

public:
    virtual void DoTask() = 0;
	//����Ҫ�ж�ʱ����
    virtual void StopTask();
	//����Ҫ�����������߼�ʱ����
    virtual bool ReExecute();
    virtual CTask* Clone();

	//��ID�Ų���ΨһID����˼��һ���������ֹͣһ����������
    int32_t GetTaskId();

private:
	void SetWaitForSendHandle(Semaphore* waitForSend);
	Semaphore* GetWaitForSendHandle();

protected:
    int32_t m_taskId;

	/* SendTask���õ��������¼�
	*/
	Semaphore* m_waitForSend;
};