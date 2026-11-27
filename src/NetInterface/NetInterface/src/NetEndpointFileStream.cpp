#include "NetEndpointFileStream.h"
#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include <algorithm>
#include <cstring>

bool NetEndpointFileStream::leaseDeadline(int64_t utcMs, uint64_t nowMs, uint32_t limitSeconds, uint64_t& expiresUtc, uint64_t& deadline)
{
    const uint64_t utcSeconds = static_cast<uint64_t>(utcMs / 1000);
    if (utcMs < 1000 || !limitSeconds || expiresUtc <= utcSeconds) { return false; }
    // 先限制秒差，避免极大的服务到期秒转毫秒时溢出。
    const uint64_t seconds = (std::min<uint64_t>)(expiresUtc - utcSeconds, limitSeconds);
    const uint64_t remainingMs = seconds * 1000 - static_cast<uint64_t>(utcMs % 1000);
    expiresUtc = utcSeconds + seconds;
    deadline = remainingMs > UINT64_MAX - nowMs ? UINT64_MAX : nowMs + remainingMs;
    return true;
}

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