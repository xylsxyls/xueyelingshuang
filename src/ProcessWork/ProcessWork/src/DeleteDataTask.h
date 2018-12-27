#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ProcessWork;
/** 删除数据内存任务
*/
class DeleteDataTask : public CTask
{
public:
	/** 构造函数
	*/
	DeleteDataTask();

public:
	/** 执行任务
	*/
	virtual void DoTask();

	/** 设置客户端
	@param [in] client 客户端
	*/
	void setClient(ProcessWork* client);

	/** 设置删除数据内存的索引值
	@param [in] index 删除数据内存的索引值
	*/
	void setDeleteDataIndex(int32_t index);

protected:
	ProcessWork* m_client;
	bool m_exit;
	HANDLE m_hCreateData;
	int32_t m_index;
};