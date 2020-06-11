#pragma once
#include "NetSenderMacro.h"
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"

class NetSenderAPI NetSender
{
protected:
	NetSender();

public:
	static NetSender& instance();

public:
	void init(ProtoMessage& message, CorrespondParam::ProtocolId protocolId, bool isServer = false);

	void send(ProtoMessage& message, bool isServer = false);
};