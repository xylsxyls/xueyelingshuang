#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class Semaphore;
class SharedMemory;
/** 接收任务
*/
class AssignTask : public CTask
{
public:
	/** 构造函数
	*/
	AssignTask();

public:
	/** 执行任务
	*/
	void DoTask();

	/** 停止任务
	*/
	void StopTask();

	/** 设置参数
	@param [in] assignSemaphore 申请信号
	@param [in] assignEndSemaphore 申请完毕信号
	@param [in] areaAssign 缓冲区号缓存队列
	*/
	void setParam(Semaphore* assignSemaphore,
		Semaphore* assignEndSemaphore,
		SharedMemory* areaAssign);

private:
	Semaphore* m_assignSemaphore;
	Semaphore* m_assignEndSemaphore;
	SharedMemory* m_areaAssign;
	std::atomic<bool> m_exit;
};