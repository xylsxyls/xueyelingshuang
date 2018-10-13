#pragma once
#include "ProcessWorkMacro.h"
#include <stdint.h>

class ProcessWorkAPI ServerReceiveCallback
{
public:
	virtual void receive(char* buffer, int32_t length) = 0;
};