#include "LibuvClient.h"
#include "uv.h"
#include <memory>

LibuvClient::LibuvClient() :
m_dest(nullptr),
m_connect(nullptr)
{
    m_isClient = true;
    std::unique_ptr<uv_tcp_t> tcp(new uv_tcp_t);
    std::unique_ptr<uv_connect_t> connect(new uv_connect_t);
    m_dest = tcp.release();
    m_connect = connect.release();
}

LibuvClient::~LibuvClient()
{
    delete m_connect;
    delete m_dest;
}