#pragma once
#include "LibuvTcpMacro.h"
#include <stdint.h>

typedef struct uv_tcp_s uv_tcp_t;
class ReceiveCallback;

class LibuvTcpAPI LibuvTcp
{
public:
	LibuvTcp();

	~LibuvTcp();

public:
	void initClient(const char* ip, int32_t port, ReceiveCallback* callback);

	void initServer(int32_t port, ReceiveCallback* callback, int32_t backlog = 128);

	void loop();

	void send(uv_tcp_t* dest, char* buffer, int32_t length);

	ReceiveCallback* callback();

protected:
	ReceiveCallback* m_receiveCallback;
};