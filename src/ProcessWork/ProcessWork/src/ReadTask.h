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
	@param [in] areaAssign 缓存区号分配缓存队列
	@param [in] areaRead 缓存区号读取缓存队列
	@param [in] memoryMap 所有缓冲区
	@param [in] copyThread 拷贝线程
	@param [in] receiveThread 接收线程
	*/
	void setParam(std::vector<ProcessReceiveCallback*>* callback,
		Semaphore* readSemaphore,
		SharedMemory* areaAssign,
		SharedMemory* areaRead,
		std::map<int32_t, std::shared_ptr<SharedMemory>>* memoryMap,
		const std::shared_ptr<CTaskThread>& copyThread,
		const std::shared_ptr<CTaskThread>& receiveThread);

private:
	std::atomic<bool> m_exit;
	std::vector<ProcessReceiveCallback*>* m_callback;
	Semaphore* m_readSemaphore;
	SharedMemory* m_areaAssign;
	SharedMemory* m_areaRead;
	std::map<int32_t, std::shared_ptr<SharedMemory>>* m_memoryMap;
	std::shared_ptr<CTaskThread> m_copyThread;
	std::shared_ptr<CTaskThread> m_receiveThread;
};