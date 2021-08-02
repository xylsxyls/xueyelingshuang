#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include "CorrespondParam/CorrespondParamAPI.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include <memory>
#include <map>
#include <atomic>
#include "NetWorkMacro.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "Area.h"

class LibuvTcp;
typedef struct uv_tcp_s uv_tcp_t;
class CTaskThread;

class NetWorkAPI NetServer : public LibuvTcp
{
	friend class ServerTask;
	friend class NetWorkHelper;
	friend class HeadTask;
public:
	NetServer();

	~NetServer();

public:
	void setFirstMessageLength(int32_t length);

	void listen(int32_t port);

	void close();

	void sendFirstMessage(uv_tcp_t* client, const char* buffer, int32_t length);

	void sendFirstMessage(uv_tcp_t* client, const std::string& message);

	void send(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type);

	void send(uv_tcp_t* client, const std::string& message, MessageType type);

	void head();

	virtual bool onFirstReceive(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type);

	virtual void onReceive(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type);
	
	virtual void onClientConnected(uv_tcp_t* client);

	virtual void onClientDisconnected(uv_tcp_t* client);

	virtual void onHeart();

	virtual int32_t headNumber();
	
protected:
	virtual void receive(uv_tcp_t* sender, const char* buffer, int32_t length);
	
	virtual void uvClientConnected(uv_tcp_t* client);

	virtual void uvClientDisconnected(uv_tcp_t* client);

	virtual void uvDisconnectedClear(uv_tcp_t* tcp);

	void loop();

	void asyncClose();

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::atomic<bool> m_isListen;
	std::vector<uv_tcp_t*> m_allClient;
	std::map<uv_tcp_t*, Area> m_receiveAreaMap;
	LockFreeQueue<char*> m_receiveQueue;
	Semaphore m_receiveSemaphore;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	uint32_t m_receiveThreadId;
	uint32_t m_headThreadId;
	uint32_t m_loopThreadId;
	int32_t m_firstMessageLength;
	int32_t m_head[2];
};