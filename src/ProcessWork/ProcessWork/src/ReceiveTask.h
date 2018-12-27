#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;
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
	virtual void DoTask();

	/** 中断任务
	*/
	virtual void StopTask();

	/** 设置客户端
	@param [in] client 客户端
	*/
	void setClient(ProcessWork* client);

private:
	HANDLE m_hAssgin;
	ProcessWork* m_client;
	bool m_exit;
};