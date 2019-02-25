#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "KeyPackage.h"

class ProcessWork;
/** 处理任务
*/
class ProcessWorkTask : public CTask
{
public:
	/** 构造函数
	*/
	ProcessWorkTask();

public:
	/** 执行任务
	*/
	virtual void DoTask();

	/** 设置钥匙
	@param [in] keyPackage 钥匙
	*/
	void setKey(const KeyPackage& keyPackage);

	/** 设置客户端
	@param [in] client 客户端
	*/
	void setClient(ProcessWork* client);

private:
	KeyPackage m_keyPackage;
	ProcessWork* m_client;
	HANDLE m_hDeleteEnd;
	bool m_exit;
};