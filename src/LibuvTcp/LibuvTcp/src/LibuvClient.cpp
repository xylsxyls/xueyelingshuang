#include "LibuvClient.h"
#include "uv.h"

LibuvClient::LibuvClient():
m_dest(nullptr),
m_connect(nullptr)
{
	m_isClient = true;
	m_dest = new uv_tcp_t;
	m_connect = new uv_connect_t;
}