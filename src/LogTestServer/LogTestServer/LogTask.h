#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"

class LogTask : public CTask
{
public:
	LogTask();

public:
	virtual void DoTask();

	std::string set4ClientId(int32_t clientId);

	void setParam(const std::string& message, int32_t clientId, const std::string& iniPath);

private:
	int32_t m_clientId;
	std::string m_strMessage;
	std::string m_iniPath;
	ProtoMessage m_message;
};