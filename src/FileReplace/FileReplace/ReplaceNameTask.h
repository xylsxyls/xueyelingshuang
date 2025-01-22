#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "ReplaceType.h"

class ReplaceNameTask : public CTask
{
public:
	/** 构造函数
	*/
	ReplaceNameTask();

public:
	/** 执行任务
	*/
	void DoTask();

	/** 设置参数
	@param [in] replaceNameParam 替换文件名任务参数
	*/
	void setParam(const ReplaceNameParam& replaceNameParam);

protected:
	//替换文件名任务参数
	ReplaceNameParam m_replaceNameParam;
};