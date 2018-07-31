#include "NetServer.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "ServerCallback.h"

class ServerCallbackBase : public ReceiveCallback
{
public:
	ServerCallbackBase();

public:
	virtual void receive(uv_tcp_t* sender, char* buffer, int32_t length);

	virtual void serverConnected(uv_tcp_t* server);

	void setCallback(ServerCallback* callback);

	void setNetServer(NetServer* netServer);

protected:
	ServerCallback* m_callback;

	NetServer* m_netServer;
};

ServerCallbackBase::ServerCallbackBase() :
m_callback(nullptr),
m_netServer(nullptr)
{
	m_callback = new ServerCallback;
}

void ServerCallbackBase::receive(uv_tcp_t* sender, char* buffer, int32_t length)
{
	if (m_callback == nullptr)
	{
		return;
	}
	m_callback->receive(sender, buffer, length);
}

void ServerCallbackBase::serverConnected(uv_tcp_t* server)
{
	if (m_callback == nullptr || m_netServer == nullptr)
	{
		return;
	}
	m_netServer->setServer(server);
	m_callback->serverConnected(server);
}

void ServerCallbackBase::setCallback(ServerCallback* callback)
{
	m_callback = callback;
}

void ServerCallbackBase::setNetServer(NetServer* netServer)
{
	m_netServer = netServer;
}

NetServer::NetServer() :
m_libuvTcp(nullptr),
m_serverCallbackBase(nullptr),
m_server(nullptr)
{
	m_libuvTcp = new LibuvTcp;
	m_serverCallbackBase = new ServerCallbackBase;
}

void NetServer::connect(const char* ip, int32_t port, ServerCallback* callback)
{
	if (m_libuvTcp == nullptr || m_serverCallbackBase == nullptr)
	{
		return;
	}
	m_serverCallbackBase->setCallback(callback);
	m_serverCallbackBase->setNetServer(this);
	m_libuvTcp->initServer(port, m_serverCallbackBase);
	m_libuvTcp->loop();
}

void NetServer::send(char* buffer, int32_t length, uv_tcp_t* dest)
{
	if (m_libuvTcp == nullptr)
	{
		return;
	}
	m_libuvTcp->send(dest, buffer, length);
}

void NetServer::setServer(uv_tcp_t* server)
{
	m_server = server;
}