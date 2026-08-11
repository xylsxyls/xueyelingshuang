#include "HttpServiceOptions.h"

HttpServiceOptions::HttpServiceOptions() :
m_listenHost("0.0.0.0"),
m_port(0),
m_workerThreads(16),
m_requestTimeoutMs(30000),
m_maxRequestBodyBytes(1024 * 1024),
m_enableKeepAlive(false),
m_enableCors(true),
m_corsAllowOrigin("*")
{

}