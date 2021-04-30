#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <vector>

class ProcessReceiveCallback;
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
	@param [in] callback 接收回调类
	@param [in] readSemaphore 读取信号
	@param [in] readEndSemaphore 读取完毕信号
	@param [in] area 缓存区号缓存
	@param [in] memoryMap 共享内存组
	@param [in] receiveThread 接收线程
	*/
	void setParam(std::vector<ProcessReceiveCallback*>* callback,
		Semaphore* readSemaphore,
		Semaphore* readEndSemaphore,
		SharedMemory* area,
		std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* memoryMap,
		const std::shared_ptr<CTaskThread>& copyThread,
		const std::shared_ptr<CTaskThread>& receiveThread);

private:
	std::atomic<bool> m_exit;
	std::vector<ProcessReceiveCallback*>* m_callback;
	Semaphore* m_readSemaphore;
	Semaphore* m_readEndSemaphore;
	SharedMemory* m_area;
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* m_memoryMap;
	std::shared_ptr<CTaskThread> m_copyThread;
	std::shared_ptr<CTaskThread> m_receiveThread;
};