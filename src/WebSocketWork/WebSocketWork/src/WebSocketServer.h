#pragma once
#include "WebSocketWorkMacro.h"
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"

struct WebSocketServerParam;

class WebSocketWorkAPI WebSocketServer
{
	friend class WebSocketServerLoopTask;
public:
	WebSocketServer();

	~WebSocketServer();

public:
	void listen(int32_t port, bool hasTls = true, bool openHttp = false);

	void run();

	void exit();

	void close(void* client, const std::string& reason);

	void send(void* client, const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

	void send(void* client, const std::string& message, MessageType type = MessageType::MESSAGE);

	virtual void onClientConnected(void* client);

	virtual void onReceive(void* client, const char* buffer, int32_t length, MessageType type);

	virtual void onClientDisconnected(void* client);

protected:
	void loop();

	void stop();

private:
	uint32_t m_loopThreadId;

public:
	WebSocketServerParam* m_server;
};