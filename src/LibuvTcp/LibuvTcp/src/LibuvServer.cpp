#include "LibuvServer.h"
#include "uv.h"

LibuvServer::LibuvServer():
m_server(nullptr)
{
	m_isClient = false;
	m_server = new uv_tcp_t;
}