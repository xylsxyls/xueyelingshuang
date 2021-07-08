#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

class Semaphore;
class SharedMemory;

/** 接收任务
*/
class ReadTask : public CTask
{
public:
	/** 构造函数
	*/
	ReadTask();

public:
	/** 执行任务
	*/
	void DoTask();

	/** 停止任务
	*/
	void StopTask();

	/** 设置参数
	@param [in] readSemaphore 读取信号
	@param [in] areaCount 缓存区个数
	@param [in] areaRead 缓存区号读取缓存队列
	@param [in] assignQueue 分配号队列
	@param [in] assignSemaphore 分配号信号量
	*/
	void setParam(Semaphore* readSemaphore,
		int32_t areaCount,
		SharedMemory* areaRead,
		LockFreeQueue<int32_t>* assignQueue,
		Semaphore* assignSemaphore);

private:
	Semaphore* m_readSemaphore;
	int32_t m_areaCount;
	SharedMemory* m_areaRead;
	LockFreeQueue<int32_t>* m_assignQueue;
	Semaphore* m_assignSemaphore;
	std::atomic<bool> m_exit;
};