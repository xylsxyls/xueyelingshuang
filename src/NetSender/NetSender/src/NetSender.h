#pragma once
#include "NetSenderMacro.h"
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"

class NetSenderAPI NetSender
{
protected:
	NetSender();

public:
	static NetSender& instance();

public:
	void init(const char* buffer = nullptr, int32_t length = 0, bool isServer = false);

	void send(const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId, bool isServer = false);
};