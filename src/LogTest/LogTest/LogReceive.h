#pragma once
#include "NetSender/NetSenderAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include <memory>
#include "ProtoMessage/ProtoMessageAPI.h"

class CTaskThread;

class LogReceive : public ProcessReceiveCallback
{
public:
	LogReceive();

	~LogReceive();

public:
	/** 接收函数
	@param [in] sendPid 发送进程ID
	@param [in] buffer 数据内存
	@param [in] length 数据长度
	@param [in] protocolId 发送数据协议
	*/
	virtual void receive(int32_t sendPid, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId);

	void receiveFromNet(char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId);

	void sendToNet(int32_t sendPid, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId);

	std::string getSenderName(int32_t sendPid);

protected:
	int32_t m_netClientManagerPid;
	std::map<int32_t, std::string> m_sendMap;
	uint32_t m_screenThreadId;
	uint32_t m_logThreadId;
	uint32_t m_logDeleteThreadId;
	uint32_t m_netThreadId;
	std::shared_ptr<CTaskThread> m_screenThread;
	std::shared_ptr<CTaskThread> m_logThread;
	std::shared_ptr<CTaskThread> m_logDeleteThread;
	std::shared_ptr<CTaskThread> m_netThread;
	ProtoMessage m_message;
	std::atomic<bool> m_needSendDeleteLog;
	std::atomic<int32_t> m_lastLogTime;
};