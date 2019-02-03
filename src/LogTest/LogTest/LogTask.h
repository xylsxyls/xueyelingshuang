#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "Variant/VariantAPI.h"

class LogTask : public CTask
{
public:
	LogTask();

public:
	virtual void DoTask();

	void setParam(bool isNet, const std::string& buffer, const std::string& processName = "");

private:
	std::string m_buffer;
	std::string m_processName;
	bool m_isNet;
	ProtoMessage m_message;
	std::map<int32_t, Variant> m_messageMap;
};