#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <vector>

class ProcessReceiveCallback;
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

	/** 设置参数
	@param [in] assign 读取的缓存区号
	@param [in] callback 接收回调类
	@param [in] areaAssign 缓存区号分配缓存队列
	@param [in] memoryMap 共享内存组
	@param [in] receiveThread 处理线程
	*/
	void setParam(int32_t assign,
		std::vector<ProcessReceiveCallback*>* callback,
		SharedMemory* areaAssign,
		std::map<int32_t, std::shared_ptr<SharedMemory>>* memoryMap,
		const std::shared_ptr<CTaskThread>& receiveThread);

private:
	int32_t m_assign;
	std::vector<ProcessReceiveCallback*>* m_callback;
	SharedMemory* m_areaAssign;
	std::map<int32_t, std::shared_ptr<SharedMemory>>* m_memoryMap;
	std::shared_ptr<CTaskThread> m_receiveThread;
};