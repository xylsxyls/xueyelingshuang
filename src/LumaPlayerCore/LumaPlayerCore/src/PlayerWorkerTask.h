#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

#include <atomic>
#include <stdint.h>

class PlayerEngine;

/** LumaPlayerCore播放调度任务，该任务放进CTaskThreadManager创建的线程中执行，控制命令打断后按最新状态重新创建
*/
class PlayerWorkerTask : public CTask
{
public:
	// 同一引擎播放调度任务类别
	static const int32_t kTaskId = 1;

	/** 构造工作任务
	@param [in] engine 播放引擎指针，任务不负责释放，必须保证线程停止前有效
	*/
	explicit PlayerWorkerTask(PlayerEngine* engine);

	/** 析构函数，不做耗时清理，线程退出由StopTask和CTaskThreadManager控制
	*/
	virtual ~PlayerWorkerTask();

public:
	/** 在线程中执行播放引擎循环
	*/
	virtual void DoTask();

	/** 请求任务退出，并唤醒等待中的工作循环
	*/
	virtual void StopTask();

	/** 唤醒工作循环，用于新命令入队或退出
	*/
	void wake();

	/** 工作线程等待一段时间或等待外部唤醒
	@param [in] timeoutMs 等待时间，单位毫秒，必须大于等于0
	@return true表示被signal唤醒，false表示超时或已经退出
	*/
	bool wait(int32_t timeoutMs);

	/** 判断外部是否请求退出
	@return true表示任务应尽快退出
	*/
	bool isExitRequested() const;

private:
	// 播放引擎指针，不负责释放
	PlayerEngine* m_engine;
	// 退出标志，由StopTask设置
	std::atomic<bool> m_exit;
	// 工作循环等待事件，命令入队、退出和状态变化时唤醒
	Semaphore m_wakeup;
};