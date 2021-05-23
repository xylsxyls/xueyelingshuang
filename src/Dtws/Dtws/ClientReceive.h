#pragma once
#include "NetSender/NetSenderAPI.h"

class ClientReceive : public ClientReceiveCallback
{
public:
	ClientReceive();

public:
	/** 初始化响应函数
	@param [in] serverId 服务进程代号
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void clientInitResponse(int32_t serverId, const char* buffer, int32_t length);

	/** 服务器消息
	@param [in] serverId 服务进程代号
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void ServerMessage(int32_t serverId, const char* buffer, int32_t length);
};