#include "NetSenderMacro.h"
#include "ClientReceiveCallback.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include <cstring>

void ClientReceiveCallback::NetClientMessage(const char* buffer, int32_t length)
{
    (void)buffer;
    (void)length;
}

void ClientReceiveCallback::NetServerMessage(const char* buffer, int32_t length)
{
    (void)buffer;
    (void)length;
}

void ClientReceiveCallback::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
{
    (void)sendPid;
    if (length < 0 || length > 1048576 || buffer == nullptr)
    {
        return;
    }
    switch (type)
    {
    case CLIENT_INIT_RESPONSE:
    {
        ProtoMessage message;
        Variant server;
        Variant information;
        if (length == 0 || !message.from(std::string(buffer, static_cast<uint32_t>(length))) ||
            !message.getKeyValue(SERVER_ID, server) || server.type() != Variant::INT32 ||
            !message.getKeyValue(CLIENT_INIT_RESPONSE_INFO, information) ||
            (information.type() != Variant::STRING && information.type() != Variant::BLOB))
        {
            return;
        }
        const std::string bytes = information.toString();
        clientInitResponse(static_cast<int32_t>(server), bytes.data(), static_cast<int32_t>(bytes.size()));
        return;
    }
    case SERVER_MESSAGE:
    {
        if (length < 8)
        {
            return;
        }
        int32_t server = 0;
        std::memcpy(&server, buffer, sizeof(server));
        ServerMessage(server, buffer + 8, length - 8);
        return;
    }
    case NET_SERVER_MESSAGE:
        if (length >= 4)
        {
            NetServerMessage(buffer + 4, length - 4);
        }
        return;
    case NET_CLIENT_MESSAGE:
        NetClientMessage(buffer, length);
        return;
    default:
        return;
    }
}