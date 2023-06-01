#pragma once
#include "CTask.h"
#include <list>
#include <map>
#include <thread>
#include <memory>
#include <mutex>
#include <atomic>

class Semaphore;

/* 线程操作指针
*/
class CTaskThreadManagerAPI CTaskThread
{
	friend class CTaskThreadManager;
private:
	/* 构造函数
	@param [in] threadId 线程ID
	*/
	CTaskThread(int32_t threadId);
	
public:
	/* 析构函数
	*/
	~CTaskThread();
	
public:
	/* 添加任务，任务优先级必须从1开始
	@param [in] spTask 任务
	@param [in] taskLevel 任务优先级
	*/
	void PostTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel = 1);

	/* 尝试添加任务，任务优先级必须从1开始，和try_lock相似
	@param [in] spTask 任务
	@param [in] taskLevel 任务优先级
	@return 返回是否添加成功
	*/
	bool TryPostTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel = 1);

	/* 同步执行任务，当任务没结束是发送线程卡住，但是任务仍然是在线程中执行
	@param [in] spTask 任务
	@param [in] taskLevel 任务优先级
	*/
	void SendTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel = 1);

	/* 判断任务队列里是否有任务
	@return 返回任务队列里是否有任务
	*/
	bool HasTask();

	/* 停止执行当前任务
	*/
	void StopCurTask();

	/* 停止执行该任务ID的任务集
	@param [in] taskId 任务ID
	@param [in] taskLevel 任务优先级，0表示所有任务范围
	*/
	void StopTask(int32_t taskId, int32_t taskLevel = 0);

	/* 停止所有任务
	*/
	void StopAllTask();

	/* 获取当前map中任务个数，该函数会清空传入的map
	@param [out] taskCountMap 左边是任务优先级，右边是当前优先级的任务个数
	*/
	void GetWaitTaskInfo(std::map<int32_t, int32_t>& taskCountMap);

	/** 获取当前任务个数
	@return 返回等待任务个数
	*/
	int32_t GetWaitTaskCount();

	/** 获取正在执行的任务优先级
	@return 返回正在执行的任务优先级
	*/
	int32_t GetCurTaskLevel();

	/** 等待所有任务执行完成后退出线程
	*/
	void WaitForEnd();

private:
	/** 创建线程
	@return 返回是否创建成功
	*/
	bool CreateThread();

	/** 销毁线程
	*/
	void DestroyThread();

	/** 设置退出信号
	*/
	void SetExitSignal();

	/** 销毁线程的时候等待线程退出
	*/
	void WaitForExit();

	/** 获取最高优先级任务
	* 如果任务队列里有任务则提出最高优先级任务，同时检查提过任务之后是否需要删除该级别队列
	*/
	void PopToCurTask();

	void StopTaskInList(const std::list<std::shared_ptr<CTask>>& taskList, int32_t taskId);

	//工作线程做三件事，执行当前任务，执行完后看任务队列里是否有任务，如果有则提出放到当前任务中，如果当前任务没有则跳过
	void WorkThread();

	void StopAllTaskUnlock();

	void HandlePostTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel);

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
	std::atomic<bool> m_hasExitSignal;

	/** 等待所有任务执行完的退出信号
	*/
	std::atomic<bool> m_waitForEndSignal;

	/* 正在执行任务的优先级
	*/
	std::atomic<int32_t> m_curTaskLevel;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    
	/* 线程ID
	*/
	int32_t m_threadId;

	/* 等待事件
	*/
	Semaphore* m_semaphore;
};
