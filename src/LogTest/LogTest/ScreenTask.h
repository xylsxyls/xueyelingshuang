#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "Variant/VariantAPI.h"

class ScreenTask : public CTask
{
public:
	ScreenTask();

public:
	virtual void DoTask();

	void setParam(bool isNet, const std::string& buffer);

private:
	std::string m_buffer;
	bool m_isNet;
	ProtoMessage m_message;
	std::map<std::string, Variant> m_messageMap;
};