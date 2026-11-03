#include "NetEndpointFileStream.h"
#include "NetInterfaceMacro.h"
#include "NetEndpointRouterState.h"
#include "NetInterface.h"
#include "NetInterfaceRequests.h"
#include "NetInterfaceRpc.h"
#include "NetInterfaceFile.h"
#include "NetInterfaceEvent.h"
#include "NetEndpointRouter.h"
#include <algorithm>
#include <cstring>
#include <ctime>
#include <utility>

NetEndpointFileStream::NetEndpointFileStream(uint64_t client, uint64_t service, uint32_t product, const NetInterfaceFileInfo& info,
                                           uint64_t deadline) :
m_id(0),
m_expiryIndex(0),
m_client(client),
m_service(service),
m_local(info.stream),
m_deadline(deadline),
m_product(product),
m_info(info)
{
    std::memset(m_previous, 0, sizeof(m_previous));
    std::memset(m_next, 0, sizeof(m_next));
}

NetEndpointFileStream::~NetEndpointFileStream()
{
    NetInterface::wipe(reinterpret_cast<uint8_t*>(&m_info), sizeof(m_info));
}

NetEndpointSecretScope::NetEndpointSecretScope(void* bytes, uint32_t size) :
m_bytes(bytes), m_size(size)
{

}

NetEndpointSecretScope::~NetEndpointSecretScope()
{
    NetInterface::wipe(static_cast<uint8_t*>(m_bytes), m_size);
}