#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "NetSenderMacro.h"

class NetSenderAPI ServerReceiveCallback : public ProcessReceiveCallback
{
public:
	/** 客户端初始化函数
	@param [in] connectId 连接ID
	@param [in] clientPid 客户端进程号
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void clientInit(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length) = 0;

	/** 服务端初始化响应函数
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void serverInitResponse(const char* buffer, int32_t length) = 0;

	/** 客户端网络管理进程消息
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void NetClientMessage(int32_t connectId, const char* buffer, int32_t length);

	/** 服务端网络管理进程消息
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void NetServerMessage(const char* buffer, int32_t length);

	/** 客户端消息
	@param [in] connectId 连接ID
	@param [in] clientPid 客户端进程号
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void ClientMessage(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length) = 0;

protected:
	/** 接收虚函数
	@param [in] sendPid 发送进程ID
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	@param [in] type 数据类型
	*/
	virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);
};