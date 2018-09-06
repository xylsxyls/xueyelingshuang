#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>
#include "CallbackBase.h"

class NetClient;
typedef struct uv_tcp_s uv_tcp_t;
class ReceiveCallback;

class NetWorkAPI ClientCallback : public CallbackBase
{
public:
	ClientCallback();

public:
	virtual void serverConnected(uv_tcp_t* server);

	void setNetClient(NetClient* netClient);

	NetClient* netClient();

protected:
	NetClient* m_netClient;
};