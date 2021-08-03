#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include <memory>
#include <string>
#include <atomic>
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "Area.h"

class CTaskThread;
typedef struct uv_tcp_s uv_tcp_t;

class NetWorkAPI NetClient : protected LibuvTcp
{
	friend class ClientTask;
	friend class NetWorkHelper;
	friend class HeartTask;
	friend class ReceiveTask;
public:
	NetClient();

	~NetClient();

public:
	void setFirstMessageLength(int32_t length);
	void connect(const char* ip, int32_t port, bool isSendHeart = true);
	void close();
	void sendFirstMessage(const char* buffer, int32_t length);
	void sendFirstMessage(const std::string& message);
	void send(const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);
	void send(const std::string& message, MessageType type = MessageType::MESSAGE);
	void heart(int32_t time = 5000);
	virtual bool onFirstReceive(const char* buffer, int32_t length, MessageType type);
	virtual void onFirstHead();
	virtual void onReceive(const char* buffer, int32_t length, MessageType type);
	virtual void onServerConnected();
	virtual void onServerNotFind();
	virtual void onServerDisconnected();
	virtual void onHeart();

protected:
	virtual void receive(uv_tcp_t* sender, const char* buffer, int32_t length);
	virtual void uvServerConnected(uv_tcp_t* server);
	virtual void uvServerNotFind();
	virtual void uvServerNotFindClear();
	virtual void uvServerDisconnected(uv_tcp_t* server);
	virtual void uvDisconnectedClear(uv_tcp_t* tcp);
	void loop();
	void asyncClose();

protected:
	uv_tcp_t* m_destServer;
	uint32_t m_receiveThreadId;
	uint32_t m_heartThreadId;
	uint32_t m_loopThreadId;
	int32_t m_firstMessageLength;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::atomic<bool> m_isConnected;
	Area m_receiveArea;
	LockFreeQueue<char*> m_receiveQueue;
	Semaphore m_receiveSemaphore;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	int32_t m_head[2];
	bool m_isSendHeart;
};