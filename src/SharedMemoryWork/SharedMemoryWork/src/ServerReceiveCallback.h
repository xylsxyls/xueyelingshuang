#pragma once
#include "SharedMemoryWorkMacro.h"
#include <stdint.h>

class SharedMemoryWorkAPI ServerReceiveCallback
{
public:
	virtual void receive(char* buffer, int32_t length) = 0;
};