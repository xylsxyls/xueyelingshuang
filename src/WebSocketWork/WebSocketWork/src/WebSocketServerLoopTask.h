#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class WebSocketServer;

class WebSocketServerLoopTask : public CTask
{
public:
	WebSocketServerLoopTask();

public:
	void DoTask();

	void StopTask();

	void setParam(WebSocketServer* server);

private:
	WebSocketServer* m_server;
};