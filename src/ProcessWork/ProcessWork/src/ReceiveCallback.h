#pragma once
#include <stdint.h>

/** 接收回调类
*/
class ReceiveCallback
{
public:
	/** 接收虚函数
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	@param [in] sendPid 发送进程ID
	@param [in] protocolId 发送数据协议
	*/
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocolId) = 0;
};