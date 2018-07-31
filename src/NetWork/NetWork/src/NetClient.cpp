#include "NetClient.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "ClientCallback.h"

class ClientCallbackBase : public ReceiveCallback
{
public:
	ClientCallbackBase();

public:
	virtual void receive(uv_tcp_t* sender, char* buffer, int32_t length);

	virtual void serverConnected(uv_tcp_t* server);

	void setCallback(ClientCallback* callback);

	void setNetClient(NetClient* netClient);

protected:
	ClientCallback* m_callback;

	NetClient* m_netClient;
};

ClientCallbackBase::ClientCallbackBase() :
m_callback(nullptr),
m_netClient(nullptr)
{
	m_callback = new ClientCallback;
}

void ClientCallbackBase::receive(uv_tcp_t* sender, char* buffer, int32_t length)
{
	if (m_callback == nullptr)
	{
		return;
	}
	m_callback->receive(sender, buffer, length);
}

void ClientCallbackBase::serverConnected(uv_tcp_t* server)
{
	if (m_callback == nullptr || m_netClient == nullptr)
	{
		return;
	}
	m_netClient->setServer(server);
	m_callback->serverConnected(server);
}

void ClientCallbackBase::setCallback(ClientCallback* callback)
{
	m_callback = callback;
}

void ClientCallbackBase::setNetClient(NetClient* netClient)
{
	m_netClient = netClient;
}

NetClient::NetClient():
m_libuvTcp(nullptr),
m_clientCallbackBase(nullptr),
m_server(nullptr)
{
	m_libuvTcp = new LibuvTcp;
	m_clientCallbackBase = new ClientCallbackBase;
}

void NetClient::connect(const char* ip, int32_t port, ClientCallback* callback)
{
	if (m_libuvTcp == nullptr || m_clientCallbackBase == nullptr)
	{
		return;
	}
	m_clientCallbackBase->setCallback(callback);
	m_clientCallbackBase->setNetClient(this);
	m_libuvTcp->initClient(ip, port, m_clientCallbackBase);
	m_libuvTcp->loop();
}

void NetClient::send(char* buffer, int32_t length, uv_tcp_t* dest)
{
	if (m_libuvTcp == nullptr)
	{
		return;
	}
	m_libuvTcp->send(dest, buffer, length);
}

void NetClient::setServer(uv_tcp_t* server)
{
	m_server = server;
}