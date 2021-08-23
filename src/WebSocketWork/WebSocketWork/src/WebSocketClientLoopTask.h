#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class WebSocketClient;

class WebSocketClientLoopTask : public CTask
{
public:
	WebSocketClientLoopTask();

public:
	void DoTask();

	void StopTask();

	void setParam(WebSocketClient* client);

private:
	WebSocketClient* m_client;
};