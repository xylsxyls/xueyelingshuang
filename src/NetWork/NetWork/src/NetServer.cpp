#include "NetServer.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "ServerCallback.h"
#include <windows.h>
#include "D:\\SendToMessageTest.h"
#include <thread>
#include <iostream>
#include "CSystem/CSystemAPI.h"
#include "NetHelper.h"

class ServerCallbackBase : public ReceiveCallback
{
public:
	ServerCallbackBase();

public:
	virtual void receive(uv_tcp_t* sender, char* buffer, int32_t length);

	virtual void clientConnected(uv_tcp_t* client);

	void setCallback(ServerCallback* callback);

protected:
	ServerCallback* m_callback;
	std::string m_area;
};

ServerCallbackBase::ServerCallbackBase():
m_callback(nullptr)
{
	m_callback = new ServerCallback;
}

void ServerCallbackBase::receive(uv_tcp_t* sender, char* buffer, int32_t length)
{
	if (m_callback == nullptr)
	{
		return;
	}

	int32_t vernier = 0;
	char* tagBuffer = nullptr;
	int32_t tagLength = 0;

	if (NetHelper::getSplitedBuffer(buffer, length, vernier, m_area, tagBuffer, tagLength) == false)
	{
		return;
	}
	
	if (!m_area.empty())
	{
		m_callback->receive(sender, tagBuffer, tagLength);
	}
	m_area.clear();

	while (NetHelper::splitBuffer(buffer, length, vernier, m_area, tagBuffer, tagLength))
	{
		m_callback->receive(sender, tagBuffer, tagLength);
	}
	return;
}

void ServerCallbackBase::clientConnected(uv_tcp_t* client)
{
	if (m_callback == nullptr)
	{
		return;
	}
	m_callback->clientConnected(client);
}

void ServerCallbackBase::setCallback(ServerCallback* callback)
{
	m_callback = callback;
}

NetServer::NetServer() :
m_libuvTcp(nullptr),
m_serverCallbackBase(nullptr)
{
	m_libuvTcp = new LibuvTcp;
	m_serverCallbackBase = new ServerCallbackBase;
}

void NetServer::listen(int32_t port, ServerCallback* callback)
{
	if (m_libuvTcp == nullptr || m_serverCallbackBase == nullptr || callback == nullptr)
	{
		return;
	}
	m_serverCallbackBase->setCallback(callback);
	callback->setNetServer(this);
	m_libuvTcp->initServer(port, m_serverCallbackBase);
	m_libuvTcp->loop();
}

void NetServer::send(char* buffer, int32_t length, uv_tcp_t* dest)
{
	if (m_libuvTcp == nullptr || dest == nullptr)
	{
		return;
	}

	char* text = (char*)::malloc(length + 4);
	*(int32_t*)text = length;
	memcpy(text + 4, buffer, length);
	m_libuvTcp->send(dest, text, length + 4);
	::free(text);
}