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
    /* 添加任务，任务优先级必须大于1
    */
    void PostTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel);

    /* 同步执行任务，当任务没结束是发送线程卡住，但是任务仍然是在线程中执行
    */
    void SendTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel);

    /* 判断任务队列里是否有任务
    */
    bool HasTask();

    void StopTask(int32_t taskId, bool ifChoke, int32_t taskLevel = 0);

    void StopAllTask();

    /* 获取当前map中任务个数，该函数会清空传入的map
    @param [out] taskCountMap 左边是任务优先级，右边是当前优先级的任务个数
    */
    void GetWaitTaskCount(std::map<int32_t, int32_t>& taskCountMap);

private:
    CTaskThread(int32_t threadId);

    bool CreateThread();

    void DestroyThread();

    /* 设置退出信号
    */
    void SetExitSignal();

    /* 销毁线程的时候等待线程退出
    */
    void WaitForExit();

    /* 获取最高优先级任务
    * 如果任务队列里有任务则提出最高优先级任务，同时检查提过任务之后是否需要删除该级别队列
    */
    void PopToCurTask();

    void StopTaskInList(const std::list<std::shared_ptr<CTask>>& taskList, int32_t taskId, bool ifChoke);

    //工作线程做三件事，执行当前任务，执行完后看任务队列里是否有任务，如果有则提出放到当前任务中，如果当前任务没有则跳过
    void WorkThread();

    void StopAllTaskUnlock();

    bool HasTaskUnlock();

private:

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    /*
    int32_t 任务优先级
    */
    std::map<int32_t, std::list<std::shared_ptr<CTask>>> m_taskMap;

    /* 正在执行的任务
    */
    std::shared_ptr<CTask> m_spCurTask;

    /* 正在执行任务的备份，用于被顶掉后重做时调用
    */
    std::shared_ptr<CTask> m_spCurTaskBk;

    /* 工作线程指针
    */
    std::unique_ptr<std::thread> m_spWorkThread;

    /* 线程锁
    */
    std::mutex m_mutex;

    /* 线程是否有退出信号
    */
    std::atomic<bool> m_hasExitSignal = false;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    
    /* 正在执行任务的优先级
    */
    int32_t m_curTaskLevel = 0;

    /* 线程ID
    */
    int32_t m_threadId = 0;

	/* 等待事件
	*/
	HANDLE m_waitEvent;
};