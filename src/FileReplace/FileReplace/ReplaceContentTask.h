#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "ReplaceType.h"

class ReplaceContentTask : public CTask
{
public:
	/** 构造函数
	*/
	ReplaceContentTask();

public:
	/** 执行任务
	*/
	void DoTask();

	/** 设置参数
	@param [in] replaceNameParam 替换内容任务参数
	*/
	void setParam(const ReplaceContentParam& replaceContentParam);

protected:
	//替换内容任务参数
	ReplaceContentParam m_replaceContentParam;
};