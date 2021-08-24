#pragma once
#include "WebSocketWorkMacro.h"
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"

struct WebSocketClientParam;

class WebSocketWorkAPI WebSocketClient
{
	friend class WebSocketClientLoopTask;
public:
	WebSocketClient();

	~WebSocketClient();

public:
	bool connect(const char* ip, int32_t port, bool hasTls = true, int32_t waitTime = 0);

	void run();

	void send(const char* buffer, int32_t length, MessageType type = MessageType::MESSAGE);

	void send(const std::string& message, MessageType type = MessageType::MESSAGE);

	void exit();

	void close(const std::string& reason);

	virtual void onServerConnected();

	virtual void onReceive(const char* buffer, int32_t length, MessageType type);

	virtual void onServerDisconnected();

protected:
	void loop();

	void stop();

private:
	uint32_t m_loopThreadId;

public:
	WebSocketClientParam* m_client;
};