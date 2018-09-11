#include "SharedMemoryServer.h"
#include "ServerReceiveCallback.h"

SharedMemoryServer::SharedMemoryServer():
m_callback(nullptr)
{

}

void SharedMemoryServer::listen(const std::string& mapName, int32_t bufferMaxLength, ServerReceiveCallback* receive)
{

}
