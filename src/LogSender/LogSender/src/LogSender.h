#pragma once
#include "LogSenderMacro.h"
#include <stdint.h>

class LogSenderAPI LogSender
{
protected:
	LogSender();

public:
	static LogSender& instance();

public:
	void send(char* buffer, int32_t length);
};