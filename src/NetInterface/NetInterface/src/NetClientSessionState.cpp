#include "NetClientSessionState.h"
#include "NetInterfaceMacro.h"
#include "NetChannel/NetChannelAPI.h"

NetClientSessionState::NetClientSessionState() :
m_serviceWindow(NET_CHANNEL_MAX_WINDOW),
m_external(nullptr),
m_providerFactory(nullptr),
m_providerDiscovery(nullptr),
m_factory(nullptr),
m_discovery(nullptr),
m_listener(nullptr),
m_tls(),
m_handshake(),
m_destination(),
m_generation(0),
m_retryAt(0),
m_recoveryDeadline(0),
m_recoveryTimeoutMs(30000),
m_initialDelayMs(250),
m_maxDelayMs(5000),
m_delay(m_initialDelayMs),
m_product(0),
m_phase(NET_CLIENT_IDLE),
m_service(false),
m_callback(false),
m_polling(false),
m_closeReason(0)
{
    m_destination.m_structSize = sizeof(m_destination);
    m_destination.m_version = 1;
}

bool NetClientSessionState::active() const
{
    return m_phase != NET_CLIENT_IDLE && m_phase != NET_CLIENT_CLOSED;
}

bool NetClientSessionState::recovering() const
{
    return m_service && (m_phase == NET_CLIENT_PENDING || m_phase == NET_CLIENT_CONNECTING ||
        m_phase == NET_CLIENT_READY || m_phase == NET_CLIENT_BACKOFF);
}

bool NetClientSessionState::expired(uint64_t nowMs) const
{
    return recovering() && m_recoveryDeadline != 0 && nowMs >= m_recoveryDeadline;
}