#pragma once
#include "NetWorkMacro.h"
#include "ReceiveCallbackBase.h"

typedef struct uv_tcp_s uv_tcp_t;
class CallbackBase;

class ClientCallbackBase : public ReceiveCallbackBase
{
public:
	virtual void serverConnected(uv_tcp_t* server);
};