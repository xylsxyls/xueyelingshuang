#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>
#include "ReceiveCallbackBase.h"

typedef struct uv_tcp_s uv_tcp_t;
class CallbackBase;

class ServerCallbackBase : public ReceiveCallbackBase
{
public:
	virtual void clientConnected(uv_tcp_t* client);
};