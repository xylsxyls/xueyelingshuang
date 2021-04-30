#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include <memory>

class CTaskThread;
typedef struct uv_tcp_s uv_tcp_t;

class NetWorkAPI NetClient : protected LibuvTcp
{
public:
	NetClient();

public:
	void connect(const char* ip, int32_t port, bool sendHeart = true);
	void send(const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);
	virtual void onReceive(const char* buffer, int32_t length, MessageType type);
	virtual void onServerConnected();
	void heart(int32_t time = 5000);
	virtual void onHeart();

protected:
	virtual void receive(uv_tcp_t* sender, const char* buffer, int32_t length);
	virtual void serverConnected(uv_tcp_t* server);

protected:
	uv_tcp_t* m_server;
	uint32_t m_receiveThreadId;
	uint32_t m_heartThreadId;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<CTaskThread> m_receiveThread;
	std::shared_ptr<CTaskThread> m_heartThread;
	std::string m_receiveArea;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};