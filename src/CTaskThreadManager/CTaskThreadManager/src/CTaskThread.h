#pragma once
#include "CTask.h"
#include <list>
#include <map>
#include <thread>
#include <memory>
#include <mutex>
#include <atomic>

class Semaphore;

/* �̲߳���ָ��
*/
class CTaskThreadManagerAPI CTaskThread
{
	friend class CTaskThreadManager;
private:
	/* ���캯��
	@param [in] threadId �߳�ID
	*/
	CTaskThread(int32_t threadId);
	
public:
	/* ��������
	*/
	~CTaskThread();
	
public:
	/* ��������������ȼ������1��ʼ
	@param [in] spTask ����
	@param [in] taskLevel �������ȼ�
	*/
	void PostTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel = 1);

	/* ������������������ȼ������1��ʼ����try_lock����
	@param [in] spTask ����
	@param [in] taskLevel �������ȼ�
	@return �����Ƿ���ӳɹ�
	*/
	bool TryPostTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel = 1);

	/* ͬ��ִ�����񣬵�����û�����Ƿ����߳̿�ס������������Ȼ�����߳���ִ��
	@param [in] spTask ����
	@param [in] taskLevel �������ȼ�
	*/
	void SendTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel = 1);

	/* �ж�����������Ƿ�������
	@return ��������������Ƿ�������
	*/
	bool HasTask();

	/* ִֹͣ�е�ǰ����
	*/
	void StopCurTask();

	/* ִֹͣ�и�����ID������
	@param [in] taskId ����ID
	@param [in] taskLevel �������ȼ���0��ʾ��������Χ
	*/
	void StopTask(int32_t taskId, int32_t taskLevel = 0);

	/* ֹͣ��������
	*/
	void StopAllTask();

	/* ��ȡ��ǰmap������������ú�������մ����map
	@param [out] taskCountMap ������������ȼ����ұ��ǵ�ǰ���ȼ����������
	*/
	void GetWaitTaskInfo(std::map<int32_t, int32_t>& taskCountMap);

	/** ��ȡ��ǰ�������
	@return ���صȴ��������
	*/
	int32_t GetWaitTaskCount();

	/** ��ȡ����ִ�е��������ȼ�
	@return ��������ִ�е��������ȼ�
	*/
	int32_t GetCurTaskLevel();

	/** �ȴ���������ִ����ɺ��˳��߳�
	*/
	void WaitForEnd();

private:
	/** �����߳�
	@return �����Ƿ񴴽��ɹ�
	*/
	bool CreateThread();

	/** �����߳�
	*/
	void DestroyThread();

	/** �����˳��ź�
	*/
	void SetExitSignal();

	/** �����̵߳�ʱ��ȴ��߳��˳�
	*/
	void WaitForExit();

	/** ��ȡ������ȼ�����
	* �����������������������������ȼ�����ͬʱ����������֮���Ƿ���Ҫɾ���ü������
	*/
	void PopToCurTask();

	void StopTaskInList(const std::list<std::shared_ptr<CTask>>& taskList, int32_t taskId);

	//�����߳��������£�ִ�е�ǰ����ִ���������������Ƿ������������������ŵ���ǰ�����У������ǰ����û��������
	void WorkThread();

	void StopAllTaskUnlock();

	void HandlePostTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel);

private:

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	/*
	int32_t �������ȼ�
	*/
	std::map<int32_t, std::list<std::shared_ptr<CTask>>> m_taskMap;

	/* ����ִ�е�����
	*/
	std::shared_ptr<CTask> m_spCurTask;

	/* ����ִ������ı��ݣ����ڱ�����������ʱ����
	*/
	std::shared_ptr<CTask> m_spCurTaskBk;

	/* �����߳�ָ��
	*/
	std::unique_ptr<std::thread> m_spWorkThread;

	/* �߳���
	*/
	std::mutex m_mutex;

	/* �߳��Ƿ����˳��ź�
	*/
	std::atomic<bool> m_hasExitSignal;

	/** �ȴ���������ִ������˳��ź�
	*/
	std::atomic<bool> m_waitForEndSignal;

	/* ����ִ����������ȼ�
	*/
	std::atomic<int32_t> m_curTaskLevel;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    
	/* �߳�ID
	*/
	int32_t m_threadId;

	/* �ȴ��¼�
	*/
	Semaphore* m_semaphore;
};
