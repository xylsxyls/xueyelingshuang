#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>

typedef struct uv_tcp_s uv_tcp_t;

class NetWorkAPI CallbackBase
{
public:
	virtual void receive(uv_tcp_t* sender, char* buffer, int32_t length, int32_t protocolId);
};