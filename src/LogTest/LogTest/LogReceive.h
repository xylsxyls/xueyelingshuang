#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include <memory>
#include "ProtoMessage/ProtoMessageAPI.h"

class CTaskThread;

class LogReceive : public ProcessReceiveCallback
{
public:
	LogReceive();

public:
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId);

	void receiveFromNet(char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId);

	void sendToNet(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId);

	std::string getSenderName(int32_t sendPid);

protected:
	int32_t m_netClientManagerPid;
	std::map<int32_t, std::string> m_sendMap;
	std::shared_ptr<CTaskThread> m_screenThread;
	std::shared_ptr<CTaskThread> m_logThread;
	std::shared_ptr<CTaskThread> m_netThread;
	ProtoMessage m_message;
};