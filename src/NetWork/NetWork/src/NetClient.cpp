#include "NetClient.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "ClientCallback.h"
#include <windows.h>
#include "NetHelper.h"
#include "D:\\SendToMessageTest.h"
#include <map>

class ClientCallbackBase : public ReceiveCallback
{
public:
	ClientCallbackBase();

public:
	virtual void receive(uv_tcp_t* sender, char* buffer, int32_t length);

	virtual void serverConnected(uv_tcp_t* server);

	void setCallback(ClientCallback* callback);

protected:
	ClientCallback* m_callback;
	std::map<uv_tcp_t*, std::string> m_area;
};

ClientCallbackBase::ClientCallbackBase():
m_callback(nullptr)
{
	m_callback = new ClientCallback;
}

void ClientCallbackBase::receive(uv_tcp_t* sender, char* buffer, int32_t length)
{
	if (m_callback == nullptr)
	{
		return;
	}

	int32_t vernier = 0;
	char* tagBuffer = nullptr;
	int32_t tagLength = 0;

	if (NetHelper::getSplitedBuffer(buffer, length, vernier, m_area[sender], tagBuffer, tagLength) == false)
	{
		return;
	}

	if (!m_area[sender].empty())
	{
		m_callback->receive(sender, tagBuffer, tagLength);
	}
	m_area[sender].clear();

	while (NetHelper::splitBuffer(buffer, length, vernier, m_area[sender], tagBuffer, tagLength))
	{
		m_callback->receive(sender, tagBuffer, tagLength);
	}
	return;
}

void ClientCallbackBase::serverConnected(uv_tcp_t* server)
{
	if (m_callback == nullptr || m_callback->m_netClient == nullptr)
	{
		return;
	}
	m_callback->m_netClient->setServer(server);
	m_callback->serverConnected(server);
}

void ClientCallbackBase::setCallback(ClientCallback* callback)
{
	m_callback = callback;
}

NetClient::NetClient() :
m_libuvTcp(nullptr),
m_clientCallbackBase(nullptr),
m_server(nullptr)
{
	m_libuvTcp = new LibuvTcp;
	m_clientCallbackBase = new ClientCallbackBase;
}

void NetClient::connect(const char* ip, int32_t port, ClientCallback* callback)
{
	if (m_libuvTcp == nullptr || m_clientCallbackBase == nullptr || callback == nullptr)
	{
		return;
	}
	m_clientCallbackBase->setCallback(callback);
	callback->setNetClient(this);
	m_libuvTcp->initClient(ip, port, m_clientCallbackBase);
	m_libuvTcp->clientLoop();
}

void NetClient::send(char* buffer, int32_t length, uv_tcp_t* dest)
{
	if (m_libuvTcp == nullptr)
	{
		return;
	}
	if (dest == nullptr)
	{
		dest = m_server;
	}
	if (m_server == nullptr)
	{
		return;
	}
	char* text = (char*)::malloc(length + 4);
	memcpy(text, &length, 4);
	memcpy(text + 4, buffer, length);
	m_libuvTcp->send(dest, text, length + 4);
	::free(text);
}

void NetClient::setServer(uv_tcp_t* server)
{
	m_server = server;
}