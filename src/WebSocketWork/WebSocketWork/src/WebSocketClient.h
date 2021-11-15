#pragma once
#include "WebSocketWorkMacro.h"
#include <stdint.h>
#include <string>

struct WebSocketClientParam;

class WebSocketWorkAPI WebSocketClient
{
	friend class WebSocketClientLoopTask;
public:
	WebSocketClient();

	~WebSocketClient();

public:
	bool connect(const char* ip, int32_t port, bool hasTls = false);

	void run();

	void send(const char* buffer, int32_t length);

	void send(const std::string& message);

	void exit();

	void close(const std::string& reason);

	virtual void onServerConnected();

	virtual void onReceive(const std::string& buffer);

	virtual void onServerDisconnected();

protected:
	void loop();

	void stop();

private:
	uint32_t m_loopThreadId;

public:
	WebSocketClientParam* m_client;
};