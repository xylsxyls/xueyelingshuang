#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "Variant/VariantAPI.h"

class NetTask : public CTask
{
public:
	virtual void DoTask();

	void setParam(const std::string& buffer, const std::string& processName, CorrespondParam::ProtocolId protocolId);

private:
	std::string m_buffer;
	std::string m_processName;
	CorrespondParam::ProtocolId m_protocolId;
	ProtoMessage m_message;
	std::map<std::string, Variant> m_messageMap;
};