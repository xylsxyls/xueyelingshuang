#include "LibuvServer.h"
#include "uv.h"

LibuvServer::LibuvServer(const LibuvPoolSettings& settings) :
LibuvBase(settings),
m_server(nullptr)
{
    m_isClient = false;
    m_server = new uv_tcp_t;
}

LibuvServer::~LibuvServer()
{
    delete m_server;
}