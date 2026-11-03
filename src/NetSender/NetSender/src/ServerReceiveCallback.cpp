#include "NetSenderMacro.h"
#include "ServerReceiveCallback.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include <cstring>

void ServerReceiveCallback::NetClientMessage(int32_t connectId, const char* buffer, int32_t length)
{
    (void)connectId;
    (void)buffer;
    (void)length;
}

void ServerReceiveCallback::NetServerMessage(const char* buffer, int32_t length)
{
    (void)buffer;
    (void)length;
}

void ServerReceiveCallback::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
{
    (void)sendPid;
    if (length < 0 || length > 1048576 || buffer == nullptr)
    {
        return;
    }
    switch (type)
    {
    case CLIENT_INIT:
    {
        ProtoMessage message;
        Variant connection;
        Variant client;
        Variant information;
        if (length == 0 || !message.from(std::string(buffer, static_cast<uint32_t>(length))) ||
            !message.getKeyValue(CONNECT_ID, connection) || connection.type() != Variant::INT32 ||
            !message.getKeyValue(CLIENT_PID, client) || client.type() != Variant::INT32 ||
            !message.getKeyValue(CLIENT_INIT_INFO, information) ||
            (information.type() != Variant::STRING && information.type() != Variant::BLOB))
        {
            return;
        }
        const std::string bytes = information.toString();
        clientInit(static_cast<int32_t>(connection), static_cast<int32_t>(client), bytes.data(), static_cast<int32_t>(bytes.size()));
        return;
    }
    case SERVER_INIT_RESPONSE:
        serverInitResponse(buffer, length);
        return;
    case NET_SERVER_MESSAGE:
        NetServerMessage(buffer, length);
        return;
    case NET_CLIENT_MESSAGE:
    {
        if (length < 4)
        {
            return;
        }
        int32_t connection = 0;
        std::memcpy(&connection, buffer, sizeof(connection));
        NetClientMessage(connection, buffer + 4, length - 4);
        return;
    }
    case MESSAGE:
    {
        if (length < 8)
        {
            return;
        }
        int32_t connection = 0;
        int32_t client = 0;
        std::memcpy(&connection, buffer, sizeof(connection));
        std::memcpy(&client, buffer + 4, sizeof(client));
        ClientMessage(connection, client, buffer + 8, length - 8);
        return;
    }
    default:
        return;
    }
}