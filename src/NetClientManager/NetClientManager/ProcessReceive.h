#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

class NetClient;

class ProcessReceive : public ProcessReceiveCallback
{
public:
	ProcessReceive();

public:
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocalId);

	void setNetClient(NetClient* netClient);

	void addClientServerLoginName(ProtoMessage& message, const std::string& clientName);

protected:
	NetClient* m_netClient;
	std::string m_computerName;
};