#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

class ProcessReceiveCallback;
class SharedMemory;

/** 接收任务
*/
class ReceiveTask : public CTask
{
public:
	/** 构造函数
	*/
	ReceiveTask();

	/** 析构函数
	*/
	~ReceiveTask();

public:
	/** 执行任务
	*/
	void DoTask();

	/** 设置参数
	@param [in] buffer 数据首地址
	@param [in] length 长度
	@param [in] sendPid 发送者的pid
	@param [in] callback 接收回调类
	@param [in] type 数据类型
	*/
	void setParam(const char* buffer,
		int32_t length,
		int32_t sendPid,
		std::vector<ProcessReceiveCallback*>* callback,
		MessageType type);

private:
	int32_t m_sendPid;
	MessageType m_type;
	std::vector<ProcessReceiveCallback*>* m_callback;
	char* m_buffer;
	int32_t m_length;
};