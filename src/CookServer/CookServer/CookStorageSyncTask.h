#pragma once

#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include <stdint.h>
#include <string>

/** MySQL状态快照同步任务
*/
class CookStorageSyncTask : public CTask
{
public:
	/** 构造函数
	*/
	CookStorageSyncTask();

	/** 设置任务参数
	@param [in] stateText 状态快照文本
	@param [in] version 状态版本
	*/
	void setParam(const std::string& stateText, int64_t version);

	/** 执行同步任务
	*/
	virtual void DoTask();

	/** 请求停止任务
	*/
	virtual void StopTask();

	/** 被高优先级任务打断后需要重做
	@return 返回true
	*/
	virtual bool ReExecute();

	/** 克隆一个带完整参数的新任务
	@return 返回新任务指针
	*/
	virtual CTask* Clone();

private:
	std::string m_stateText;
	int64_t m_version;
	std::atomic<bool> m_stopped;
};
