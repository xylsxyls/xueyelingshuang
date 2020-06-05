#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessReceiveCallback;
class SharedMemory;

/** 接收任务
*/
class ReceiveTask : public CTask
{
public:
	/** 构造函数
	*/
	ReceiveTask();

public:
	/** 执行任务
	*/
	void DoTask();

	/** 设置参数
	@param [in] assign 读取的缓存区号
	@param [in] callback 接收回调类
	@param [in] memoryMap 共享内存组
	*/
	void setParam(int32_t assign,
		ProcessReceiveCallback* callback,
		std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* memoryMap);

private:
	int32_t m_assign;
	ProcessReceiveCallback* m_callback;
	std::map<int32_t, std::pair<std::shared_ptr<SharedMemory>, std::shared_ptr<std::atomic<bool>>>>* m_memoryMap;
};