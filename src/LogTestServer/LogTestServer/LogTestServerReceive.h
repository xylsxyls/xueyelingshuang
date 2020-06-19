#pragma once
#include "NetSender/NetSenderAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include <memory>

class CTaskThread;

class LogTestServerReceive : public ProcessReceiveCallback
{
public:
	LogTestServerReceive();

public:
	/** 接收函数
	@param [in] sendPid 发送进程ID
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	@param [in] protocolId 发送数据协议
	*/
	virtual void receive(int32_t sendPid, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId);

private:
	std::string m_iniPath;
};