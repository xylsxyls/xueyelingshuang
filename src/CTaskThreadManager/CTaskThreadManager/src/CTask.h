#pragma once
#include "CTaskThreadManagerMacro.h"
#include <stdint.h>
#include <windows.h>

//���Ҫ�������ִ�н���������ڴ���task��ʱ��Ͱѻ�ý����ָ�봫��task�ڲ����̰߳�ȫ�����ⲿά��
class CTaskThreadManagerAPI CTask
{
	friend class CTaskThread;
public:
	CTask();
    CTask(int32_t taskId);

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
	void SetWaitForSendHandle(HANDLE waitForSend);
	HANDLE GetWaitForSendHandle();

protected:
    int32_t m_taskId;

	/* SendTask���õ��������¼�
	*/
	HANDLE m_waitForSend = nullptr;
};