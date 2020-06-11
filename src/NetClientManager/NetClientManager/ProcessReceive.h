#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

class Client;

class ProcessReceive : public ProcessReceiveCallback
{
public:
	ProcessReceive();

public:
	/** 接收函数
	@param [in] sendPid 发送进程ID
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	@param [in] protocolId 发送数据协议
	*/
	virtual void receive(int32_t sendPid, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId);

	/** 设置客户端
	@param [in] client 客户端
	*/
	void setClient(Client* client);

protected:
	Client* m_client;
	std::string m_computerName;
};