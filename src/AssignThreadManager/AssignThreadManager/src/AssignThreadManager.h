#pragma once
#include "AssignThreadManagerMacro.h"
#include <stdint.h>
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

/** 多人分发任务的线程管理者
*/
class AssignThreadManagerAPI AssignThreadManager
{
protected:
	/** 构造函数
	*/
	AssignThreadManager();

public:
	/** 单一实例
	@return 返回单一实例
	*/
	static AssignThreadManager& instance();

public:
	/** 初始化
	@param [in] 分发者个数
	*/
	void init(int32_t assignCount);

	/** 向入口线程传入分发任务
	@param [in] vecAssignTask 分发任务
	*/
	void postAssignTask(const std::vector<std::shared_ptr<CTask>>& vecAssignTask);
	
	/** 向执行线程传入执行任务
	@param [in] spTask 执行任务
	@param [in] taskLevel 任务优先级
	*/
	void postTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel = 1);

	/** 停止所有任务
	*/
	void stopAllTask();

	/** 反初始化
	*/
	void uninit();

protected:
	//入口线程
	uint32_t m_enterThreadId;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//分配线程集合
	std::vector<uint32_t> m_vecAssignThreadId;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	//执行线程
	uint32_t m_threadId;
};