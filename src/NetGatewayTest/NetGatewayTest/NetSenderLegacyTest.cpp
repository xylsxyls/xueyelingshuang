#include "NetSenderTestAccess.h"
#include "NetSenderLegacyTest.h"
#include <climits>
#include <cstdio>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#endif

NetSenderLegacyClientProbe::NetSenderLegacyClientProbe() :
m_calls(0)
{

}

void NetSenderLegacyClientProbe::inject(MessageType type, const char* bytes, int32_t size)
{
    receive(19, bytes, size, type);
}

void NetSenderLegacyClientProbe::clientInitResponse(int32_t serverId, const char* bytes, int32_t size)
{
    (void)serverId;
    (void)bytes;
    (void)size;
    ++m_calls;
}

void NetSenderLegacyClientProbe::ServerMessage(int32_t serverId, const char* bytes, int32_t size)
{
    (void)serverId;
    (void)bytes;
    (void)size;
    ++m_calls;
}

NetSenderLegacyServerProbe::NetSenderLegacyServerProbe() :
m_calls(0)
{

}

void NetSenderLegacyServerProbe::inject(MessageType type, const char* bytes, int32_t size)
{
    receive(19, bytes, size, type);
}

void NetSenderLegacyServerProbe::clientInit(int32_t connection, int32_t pid, const char* bytes, int32_t size)
{
    (void)connection;
    (void)pid;
    (void)bytes;
    (void)size;
    ++m_calls;
}

void NetSenderLegacyServerProbe::serverInitResponse(const char* bytes, int32_t size)
{
    (void)bytes;
    (void)size;
    ++m_calls;
}

void NetSenderLegacyServerProbe::ClientMessage(int32_t connection, int32_t pid, const char* bytes, int32_t size)
{
    (void)connection;
    (void)pid;
    (void)bytes;
    (void)size;
    ++m_calls;
}