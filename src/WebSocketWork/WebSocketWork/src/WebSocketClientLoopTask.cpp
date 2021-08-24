#include "WebSocketClientLoopTask.h"
#include "WebSocketClient.h"

WebSocketClientLoopTask::WebSocketClientLoopTask():
m_client(nullptr)
{

}

void WebSocketClientLoopTask::DoTask()
{
	m_client->loop();
}

void WebSocketClientLoopTask::StopTask()
{
	m_client->stop();
}

void WebSocketClientLoopTask::setParam(WebSocketClient* client)
{
	m_client = client;
}