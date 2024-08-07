#pragma once
#include <stdint.h>
#include "LibuvTcpMacro.h"

typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_loop_s uv_loop_t;

struct LibuvBase;

class LibuvTcpAPI LibuvTcp
{
public:
	LibuvTcp();

	virtual ~LibuvTcp();

public:
	bool initClient(const char* ip, int32_t port);

	bool initServer(int32_t port, int32_t backlog = 128);

	void loop();

	void stop();

	//申请send空间
	char* memoryForSend(int32_t length);

	//需保证uv_tcp_t*为第一个，length为第二个，后面是包体，length指的是后面包体的长度，不包括length本身长度
	void send(const char* buffer);

	uv_loop_t* loopPtr();

	bool isClient();

	void close(uv_tcp_t* tcp);

	virtual void receive(uv_tcp_t* sender, const char* buffer, int32_t length);

	virtual void uvClientConnected(uv_tcp_t* client);

	//执行完该函数之后会关闭连接
	virtual void uvClientDisconnected(uv_tcp_t* client);

	virtual void uvServerConnected(uv_tcp_t* server);

	virtual void uvServerNotFind();

	virtual void uvServerNotFindClear();

	//网络断开10秒后会提示断开
	virtual void uvServerDisconnected(uv_tcp_t* server);

	virtual void uvDisconnectedClear(uv_tcp_t* tcp);

public:
	LibuvBase* m_libuv;
};