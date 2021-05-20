#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "NetSenderMacro.h"

class NetSenderAPI ClientReceiveCallback : public ProcessReceiveCallback
{
public:
	/** 初始化响应函数
	@param [in] serverId 服务进程代号
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void clientInitResponse(int32_t serverId, const char* buffer, int32_t length) = 0;

	/** 客户端网络管理进程消息
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void NetClientMessage(const char* buffer, int32_t length);

	/** 服务端网络管理进程消息
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void NetServerMessage(const char* buffer, int32_t length);

	/** 服务器消息
	@param [in] serverId 服务进程代号
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void ServerMessage(int32_t serverId, const char* buffer, int32_t length) = 0;

protected:
	/** 接收虚函数
	@param [in] sendPid 发送进程ID
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	@param [in] type 数据类型
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);
};