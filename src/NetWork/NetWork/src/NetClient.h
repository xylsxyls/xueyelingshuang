#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include <memory>
#include <string>
#include <atomic>

class CTaskThread;
typedef struct uv_tcp_s uv_tcp_t;

class NetWorkAPI NetClient : protected LibuvTcp
{
	friend class ClientTask;
public:
	NetClient();

	~NetClient();

public:
	void connect(const char* ip, int32_t port, bool isSendHeart = true);
	void close();
	void send(const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);
	virtual void onReceive(const char* buffer, int32_t length, MessageType type);
	virtual void onServerConnected();
	virtual void onServerNotFind();
	virtual void onServerDisconnected();
	void heart(int32_t time = 5000);
	virtual void onHeart();

protected:
	virtual void receive(uv_tcp_t* sender, const char* buffer, int32_t length);
	virtual void uvServerConnected(uv_tcp_t* server);
	virtual void uvServerNotFind();
	virtual void uvServerNotFindClear();
	virtual void uvServerDisconnected(uv_tcp_t* server);
	virtual void uvDisconnectedClear(uv_tcp_t* tcp);
	void loop();

protected:
	uv_tcp_t* m_destServer;
	uint32_t m_receiveThreadId;
	uint32_t m_heartThreadId;
	uint32_t m_loopThreadId;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::atomic<bool> m_isConnected;
	std::string m_receiveArea;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};