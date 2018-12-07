#include "ProcessReceive.h"
#include "NetWork/NetWorkAPI.h"

ProcessReceive::ProcessReceive():
m_netClient(nullptr)
{

}

void ProcessReceive::receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocalId)
{
	m_netClient->send(buffer, length);
}

void ProcessReceive::setNetClient(NetClient* netClient)
{
	m_netClient = netClient;
}

