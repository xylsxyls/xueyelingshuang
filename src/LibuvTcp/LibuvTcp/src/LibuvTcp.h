#pragma once
#include "LibuvTcpMacro.h"
#include <stdint.h>

class LibuvTcp;
typedef struct uv_tcp_s uv_tcp_t;
class ReceiveCallback
{
public:
	//ReceiveCallback();
public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length){}

	virtual void clientConnected(uv_tcp_t* client){}

	virtual void serverConnected(uv_tcp_t* server){}

	LibuvTcp* m_libuvTcp;
};

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