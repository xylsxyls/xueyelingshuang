#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

class Semaphore;
class SharedMemory;

/** 拷贝任务
*/
class CopyTask : public CTask
{
public:
	/** 构造函数
	*/
	CopyTask();

public:
	/** 执行任务
	*/
	void DoTask();

	/** 中断任务
	*/
	void StopTask();

	/** 设置参数
	@param [in] areaAssign 缓冲区号分配队列
	@param [in] memoryMap 共享内存组
	@param [in] assignQueue 分配号队列
	@param [in] assignSemaphore 分配号信号量
	@param [in] receiveQueue 接收队列
	@param [in] receiveSemaphore 接收信号量
	*/
	void setParam(SharedMemory* areaAssign,
		std::map<int32_t, std::shared_ptr<SharedMemory>>* memoryMap,
		LockFreeQueue<int32_t>* assignQueue,
		Semaphore* assignSemaphore,
		LockFreeQueue<char*>* receiveQueue,
		Semaphore* receiveSemaphore);

private:
	SharedMemory* m_areaAssign;
	std::map<int32_t, std::shared_ptr<SharedMemory>>* m_memoryMap;
	LockFreeQueue<int32_t>* m_assignQueue;
	Semaphore* m_assignSemaphore;
	LockFreeQueue<char*>* m_receiveQueue;
	Semaphore* m_receiveSemaphore;
	std::atomic<bool> m_exit;
};