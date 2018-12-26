#pragma once
#include <stdint.h>
#include <map>

class ProcessReadWriteMutex;

/** 进程锁管理类
*/
class ProcessMutexManager
{
protected:
	/** 构造函数
	*/
	ProcessMutexManager();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static ProcessMutexManager& instance();

	/** 获取位置进程锁
	@param [in] pid 进程ID
	@return 返回位置进程锁
	*/
	ProcessReadWriteMutex* positionMutex(int32_t pid);

protected:
	std::map<int32_t, ProcessReadWriteMutex*> m_positionMutex;
};