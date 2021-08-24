#include "WebSocketServerLoopTask.h"
#include "WebSocketServer.h"

WebSocketServerLoopTask::WebSocketServerLoopTask():
m_server(nullptr)
{

}

void WebSocketServerLoopTask::DoTask()
{
	m_server->loop();
}

void WebSocketServerLoopTask::StopTask()
{
	m_server->stop();
}

void WebSocketServerLoopTask::setParam(WebSocketServer* server)
{
	m_server = server;
}