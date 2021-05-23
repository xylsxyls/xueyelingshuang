#include "ServerReceive.h"
#include "ClientLineManager.h"

ServerReceive::ServerReceive()
{

}

void ServerReceive::clientInit(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length)
{
	printf("Dtws server receive = %s\n", buffer);
	ClientLineManager::instance().addClient(buffer, connectId, clientPid);
	NetSender::instance().sendClientInitResponse(connectId, clientPid, buffer);
}

void ServerReceive::serverInitResponse(const char* buffer, int32_t length)
{
	printf("Dtws server init, buffer = %s\n", buffer);
}

void ServerReceive::ClientMessage(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length)
{

}