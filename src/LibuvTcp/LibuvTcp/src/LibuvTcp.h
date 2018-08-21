#pragma once
#include "LibuvTcpMacro.h"
#include <stdint.h>
#include <vector>
#include <mutex>

typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_loop_s uv_loop_t;
class ReceiveCallback;

class LibuvTcpAPI LibuvTcp
{
public:
	LibuvTcp();

	~LibuvTcp();

public:
	void initClient(const char* ip, int32_t port, ReceiveCallback* callback);

	void initServer(int32_t port, ReceiveCallback* callback, int32_t backlog = 128);

	void clientLoop();

	void serverLoop();

	void send(uv_tcp_t* dest, char* buffer, int32_t length);

	ReceiveCallback* callback();

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<uv_loop_t*> m_vecServerLoop;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	uv_loop_t* m_clientLoop;
	int32_t m_workIndex;
	int32_t m_coreCount;
	std::mutex m_mu;

protected:
	ReceiveCallback* m_receiveCallback;
};