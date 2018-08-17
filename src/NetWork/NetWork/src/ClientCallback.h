#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>
#include "CallbackBase.h"

class NetClient;
typedef struct uv_tcp_s uv_tcp_t;
class ReceiveCallback;

class NetWorkAPI ClientCallback : public CallbackBase
{
	friend class ClientCallbackBase;
public:
	ClientCallback();

public:
	virtual void serverConnected(uv_tcp_t* server);

	void setNetClient(NetClient* netClient);

protected:
	NetClient* m_netClient;

private:
	ReceiveCallback* m_callback;
};