#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

/** 发送任务
*/
class SendTask : public CTask
{
public:
	/** 构造函数
	*/
	SendTask();

	/** 析构函数
	*/
	~SendTask();

public:
	/** 执行任务
	*/
	void DoTask();

	/** 设置参数
	@param [in] buffer 数据首地址
	@param [in] length 长度
	@param [in] thisProcessId 当前进程pid
	@param [in] destPid 目标进程pid
	@param [in] processName 目标进程名，不带后缀
	@param [in] type 数据类型
	*/
	void setParam(const char* buffer,
		int32_t length,
		int32_t thisProcessId,
		int32_t destPid,
		const std::string& processName,
		MessageType type);

private:
	int32_t m_thisProcessPid;
	int32_t m_destPid;
	std::string m_processName;
	char* m_buffer;
	int32_t m_length;
	MessageType m_type;
};