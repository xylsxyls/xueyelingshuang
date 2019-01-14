#pragma once
#include "NetSenderMacro.h"
#include <stdint.h>

class NetSenderAPI NetSender
{
public:
	enum ProtocolId
	{
		INIT = 0,
		PROTO_MESSAGE,
		JSON,
		XML
	};

protected:
	NetSender();

public:
	static NetSender& instance();

public:
	void send(const char* buffer, int32_t length, int32_t protocolId, bool isServer = false);

private:
	bool m_isInit;
};