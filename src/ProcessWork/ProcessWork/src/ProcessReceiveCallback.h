#pragma once
#include <stdint.h>
#include "ProcessWorkMacro.h"

/** 接收回调类
*/
class ProcessWorkAPI ProcessReceiveCallback
{
public:
	/** 接收虚函数
	@param [in] sendPid 发送进程ID
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	@param [in] type 数据类型
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type) = 0;
};