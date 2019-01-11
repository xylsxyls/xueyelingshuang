#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;
class SharedMemory;
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

	/** 设置要删除的内存指针
	@param [in] deleteMemory 要删除的内存指针
	*/
	void setDeleteMemory(SharedMemory* deleteMemory);

protected:
	ProcessWork* m_client;
	bool m_exit;
	HANDLE m_hCreateData;
	SharedMemory* m_deleteMemory;
};