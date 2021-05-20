#pragma once
#include "NetSender/NetSenderAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include <memory>

class CTaskThread;

class LogTestServerReceive : public ServerReceiveCallback
{
public:
	/** 构造函数
	*/
	LogTestServerReceive();

public:
	/** 客户端初始化函数
	@param [in] connectId 连接ID
	@param [in] clientPid 客户端进程号
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void clientInit(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length);

	/** 服务端初始化响应函数
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void serverInitResponse(const char* buffer, int32_t length);

	/** 客户端消息
	@param [in] connectId 连接ID
	@param [in] clientPid 客户端进程号
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	*/
	virtual void ClientMessage(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length);

private:
	std::string m_iniPath;
};