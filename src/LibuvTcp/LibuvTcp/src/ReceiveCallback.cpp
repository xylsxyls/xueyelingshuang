#include "ReceiveCallback.h"

ReceiveCallback::ReceiveCallback():
m_libuvTcp(nullptr)
{

}

void ReceiveCallback::receive(uv_tcp_t* client, char* buffer, int32_t length)
{

}

void ReceiveCallback::clientConnected(uv_tcp_t* client)
{

}

void ReceiveCallback::serverConnected(uv_tcp_t* server)
{

}