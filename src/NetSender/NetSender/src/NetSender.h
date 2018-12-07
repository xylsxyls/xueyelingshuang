#pragma once
#include "NetSenderMacro.h"
#include <stdint.h>

class NetSenderAPI NetSender
{
protected:
	NetSender();

public:
	static NetSender& instance();

public:
	void send(const char* buffer, int32_t length);
};