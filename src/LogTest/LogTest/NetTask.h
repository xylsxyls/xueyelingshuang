#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "Variant/VariantAPI.h"

class NetTask : public CTask
{
public:
	NetTask();

public:
	virtual void DoTask();

	void setParam(const std::string& buffer, const std::string& processName, const std::string& loginName);

private:
	std::string m_buffer;
	std::string m_processName;
	std::string m_loginName;
	bool m_isDeal;
	ProtoMessage m_message;
	std::map<int32_t, Variant> m_messageMap;
};