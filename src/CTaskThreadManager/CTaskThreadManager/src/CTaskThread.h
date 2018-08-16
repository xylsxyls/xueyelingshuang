#pragma once
#include "CTask.h"
#include <list>
#include <map>
#include <thread>
#include <memory>
#include <mutex>
#include <atomic>

class CTaskThreadManagerAPI CTaskThread
{
    friend class CTaskThreadManager;
public:
    /* ��������������ȼ������1��ʼ
    */
    void PostTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel);

	/* ������������������ȼ������1��ʼ����try_lock����
	*/
	bool TryPostTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel);

    /* ͬ��ִ�����񣬵�����û�����Ƿ����߳̿�ס������������Ȼ�����߳���ִ��
    */
    void SendTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel);

    /* �ж�����������Ƿ�������
    */
    bool HasTask();

	void StopCurTask();

    void StopTask(int32_t taskId, int32_t taskLevel = 0);

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
	*/
	int32_t GetCurTaskLevel();

private:
    CTaskThread(int32_t threadId);

    bool CreateThread();

    void DestroyThread();

    /* �����˳��ź�
    */
    void SetExitSignal();

    /* �����̵߳�ʱ��ȴ��߳��˳�
    */
    void WaitForExit();

    /* ��ȡ������ȼ�����
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
    std::atomic<bool> m_hasExitSignal = false;

	/* ����ִ����������ȼ�
	*/
	std::atomic<int32_t> m_curTaskLevel = 0;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    
    /* �߳�ID
    */
    int32_t m_threadId = 0;

	/* �ȴ��¼�
	*/
	HANDLE m_waitEvent;
};