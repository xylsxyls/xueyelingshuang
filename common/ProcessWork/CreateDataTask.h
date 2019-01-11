#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;
/** 创建数据内存任务
*/
class CreateDataTask : public CTask
{
public:
	/** 构造函数
	*/
	CreateDataTask();

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

protected:
	ProcessWork* m_client;
	bool m_exit;
	HANDLE m_hCreateData;
};