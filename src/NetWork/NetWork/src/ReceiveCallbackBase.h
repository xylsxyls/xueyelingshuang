#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>
#include "LibuvTcp/LibuvTcpAPI.h"

typedef struct uv_tcp_s uv_tcp_t;
class CallbackBase;

class ReceiveCallbackBase : public ReceiveCallback
{
public:
	ReceiveCallbackBase();

public:
	virtual void receive(uv_tcp_t* sender, char* buffer, int32_t length);

	void setCallback(CallbackBase* callback);

protected:
	CallbackBase* m_callback;
	std::map<uv_tcp_t*, std::string> m_area;
};