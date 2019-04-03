#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include <memory>

class CTaskThread;

class LogTestServerReceive : public ProcessReceiveCallback
{
public:
	LogTestServerReceive();

public:
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId);

	int32_t getClientId(const char* buffer, int32_t length);

	std::string set4ClientId(int32_t clientId);

private:
	std::string m_iniPath;
	std::shared_ptr<CTaskThread> m_logThread;
};