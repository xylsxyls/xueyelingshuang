#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;
/** 删除钥匙内存任务
*/
class DeleteKeyTask : public CTask
{
public:
	/** 构造函数
	*/
	DeleteKeyTask();

public:
	/** 执行任务
	*/
	virtual void DoTask();

	/** 设置客户端
	@param [in] client 客户端
	*/
	void setClient(ProcessWork* client);

protected:
	ProcessWork* m_client;
	bool m_exit;
	HANDLE m_hCreateData;
};