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
	@param [in] area 缓冲区号缓存
	@param [in] memoryMap 共享内存组
	*/
	void setParam(Semaphore* assignSemaphore,
		Semaphore* assignEndSemaphore,
		SharedMemory* area,
		std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* memoryMap);

private:
	std::atomic<bool> m_exit;
	Semaphore* m_assignSemaphore;
	Semaphore* m_assignEndSemaphore;
	SharedMemory* m_area;
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* m_memoryMap;
};