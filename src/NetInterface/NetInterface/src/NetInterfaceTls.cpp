#include "NetInterfaceTls.h"
#include "NetInterfaceFileGuard.h"
#include "NetExecution.h"
#include "CEncodeDecode/CEncodeDecodeAPI.h"


// 两层返回值契约须一致，防止未来新增状态时适配层静默误映射
static_assert(static_cast<int>(NET_INTERFACE_TLS_OK) == static_cast<int>(CENCODE_TLS_OK), "TLS status mapping");
static_assert(static_cast<int>(NET_INTERFACE_TLS_HANDSHAKE) == static_cast<int>(CENCODE_TLS_HANDSHAKE), "TLS status mapping");
static_assert(static_cast<int>(NET_INTERFACE_TLS_READY) == static_cast<int>(CENCODE_TLS_READY), "TLS status mapping");
static_assert(static_cast<int>(NET_INTERFACE_TLS_CLOSED) == static_cast<int>(CENCODE_TLS_CLOSED), "TLS status mapping");
static_assert(static_cast<int>(NET_INTERFACE_TLS_WANT_IO) == static_cast<int>(CENCODE_TLS_WANT_IO), "TLS status mapping");
static_assert(static_cast<int>(NET_INTERFACE_TLS_ARGUMENT) == static_cast<int>(CENCODE_TLS_ARGUMENT), "TLS status mapping");
static_assert(static_cast<int>(NET_INTERFACE_TLS_MEMORY) == static_cast<int>(CENCODE_TLS_MEMORY), "TLS status mapping");
static_assert(static_cast<int>(NET_INTERFACE_TLS_PROTOCOL) == static_cast<int>(CENCODE_TLS_PROTOCOL), "TLS status mapping");
static_assert(static_cast<int>(NET_INTERFACE_TLS_TIMEOUT) == static_cast<int>(CENCODE_TLS_TIMEOUT), "TLS status mapping");
static_assert(static_cast<int>(NET_INTERFACE_TLS_TRUNCATED) == static_cast<int>(CENCODE_TLS_TRUNCATED), "TLS status mapping");
static_assert(static_cast<int>(NET_INTERFACE_TLS_STATE) == static_cast<int>(CENCODE_TLS_STATE), "TLS status mapping");

/** 检查加密引擎当前是否有宿主可立即取出的密文，仅检查连续前缀
@param [in] tls 已创建的加密通道
@return 密文队列有可交付数据
*/
static bool HasTlsCiphertext(const CEncodeDecodeTls& tls)
{
    const uint8_t* bytes = nullptr;
    uint32_t size = 0;
    tls.peekOutput(bytes, size);
    return size != 0;
}

NetInterfaceTls::NetInterfaceTls(const NetInterfaceTlsOptions& options) :
NetInterfaceTls(options, CEncodeDecodeTlsQueueLimits())
{

}

NetInterfaceTls::NetInterfaceTls(const NetInterfaceTlsOptions& options, const CEncodeDecodeTlsQueueLimits& limits) :
m_tls(nullptr),
m_handshakeStart(0),
m_handshakeTimeout(options.effectiveHandshakeTimeout()),
m_handshakeStarted(false),
m_inputFinished(false),
m_lastClose(NET_INTERFACE_TLS_STATE)
{
    CEncodeDecodeTlsOptions crypto = {};
    crypto.server = options.server;
    crypto.authority = options.authority;
    crypto.authoritySize = options.authoritySize;
    crypto.certificate = options.certificate;
    crypto.certificateSize = options.certificateSize;
    crypto.privateKey = options.privateKey;
    crypto.privateKeySize = options.privateKeySize;
    crypto.hostname = options.hostname;
    crypto.entropy = options.entropy;
    crypto.entropyContext = options.entropyContext;
    crypto.handshakeTimeoutMs = options.handshakeTimeoutMs;
    m_tls = CEncodeDecode::createTls(crypto, limits);
}

NetInterfaceTls::~NetInterfaceTls()
{
    CEncodeDecode::destroyTls(m_tls);
}

int32_t NetInterfaceTls::feed(const uint8_t* data, uint32_t size, uint32_t& consumed)
{
    if (!NetInterfaceFileGuard::prepareCount(data, size, &consumed)) { return NET_INTERFACE_TLS_ARGUMENT; }
    const int32_t result = m_tls ? m_tls->feed(data, size, consumed) : NET_INTERFACE_TLS_MEMORY;
    if (result >= 0 && consumed != 0) { NetExecution::progress(); }
    return result;
}

int32_t NetInterfaceTls::progress(uint64_t monotonicMs)
{
    const int32_t result = m_tls ? m_tls->progress(monotonicMs) : NET_INTERFACE_TLS_MEMORY;
    if (result == NET_INTERFACE_TLS_HANDSHAKE)
    {
        if (!m_handshakeStarted)
        {
            m_handshakeStart = monotonicMs;
            m_handshakeStarted = true;
        }
        // 引擎已校验时间未倒退；按经过时间换算，起点靠近UINT64_MAX也不会溢出。
        NetExecution::deadline(m_handshakeTimeout, monotonicMs - m_handshakeStart);
    }
    return result;
}

int32_t NetInterfaceTls::peekOutput(const uint8_t*& data, uint32_t& size) const
{
    if (NetInterfaceFileGuard::overlap(&data, sizeof(data), &size, sizeof(size))) { return NET_INTERFACE_TLS_ARGUMENT; }
    data = nullptr;
    size = 0;
    return m_tls ? m_tls->peekOutput(data, size) : NET_INTERFACE_TLS_MEMORY;
}

int32_t NetInterfaceTls::consumeOutput(uint32_t size)
{
    const int32_t result = m_tls ? m_tls->consumeOutput(size) : NET_INTERFACE_TLS_MEMORY;
    if (result == NET_INTERFACE_TLS_OK && size != 0) { NetExecution::progress(); }
    return result;
}

int32_t NetInterfaceTls::write(const uint8_t* data, uint32_t size, uint32_t& accepted)
{
    if (!NetInterfaceFileGuard::prepareCount(data, size, &accepted)) { return NET_INTERFACE_TLS_ARGUMENT; }
    const int32_t result = m_tls ? m_tls->write(data, size, accepted) : NET_INTERFACE_TLS_MEMORY;
    if (accepted != 0) { NetExecution::progress(); }
    return result;
}

int32_t NetInterfaceTls::append(const uint8_t* data, uint32_t size, uint32_t& accepted)
{
    if (!NetInterfaceFileGuard::prepareCount(data, size, &accepted)) { return NET_INTERFACE_TLS_ARGUMENT; }
    const int32_t result = m_tls ? m_tls->append(data, size, accepted) : NET_INTERFACE_TLS_MEMORY;
    if (accepted != 0) { NetExecution::progress(); }
    return result;
}

int32_t NetInterfaceTls::appendExact(const uint8_t* data, uint32_t size)
{
    return appendExact(data, size, nullptr, 0);
}

int32_t NetInterfaceTls::appendExact(const uint8_t* first, uint32_t firstSize, const uint8_t* second, uint32_t secondSize)
{
    if (!m_tls) { return NET_INTERFACE_TLS_MEMORY; }
    const bool emptyOutput = NetExecution::current() && !HasTlsCiphertext(*m_tls);
    const int32_t result = m_tls->appendExact(first, firstSize, second, secondSize);
    if ((result == NET_INTERFACE_TLS_OK && (firstSize || secondSize)) ||
        (result == NET_INTERFACE_TLS_WANT_IO && emptyOutput && HasTlsCiphertext(*m_tls)))
    {
        NetExecution::progress();
    }
    return result;
}

int32_t NetInterfaceTls::read(uint8_t* output, uint32_t capacity, uint32_t& received)
{
    if (!NetInterfaceFileGuard::prepareCount(output, capacity, &received)) { return NET_INTERFACE_TLS_ARGUMENT; }
    const int32_t result = m_tls ? m_tls->read(output, capacity, received) : NET_INTERFACE_TLS_MEMORY;
    if (received != 0) { NetExecution::progress(); }
    return result;
}

int32_t NetInterfaceTls::finishInput()
{
    bool first = false;
    return finishInput(first);
}

int32_t NetInterfaceTls::finishInput(bool& first)
{
    const int32_t result = m_tls ? m_tls->finishInput() : NET_INTERFACE_TLS_MEMORY;
    first = result == NET_INTERFACE_TLS_OK && !m_inputFinished && m_tls->status() != NET_INTERFACE_TLS_CLOSED;
    if (first)
    {
        m_inputFinished = true;
        NetExecution::progress();
    }
    return result;
}

int32_t NetInterfaceTls::close()
{
    if (!m_tls) { return NET_INTERFACE_TLS_MEMORY; }
    const bool retryEmpty = m_lastClose == NET_INTERFACE_TLS_WANT_IO && NetExecution::current() && !HasTlsCiphertext(*m_tls);
    const int32_t result = m_tls->close();
    if (result == NET_INTERFACE_TLS_OK || result == NET_INTERFACE_TLS_WANT_IO)
    {
        if (m_lastClose != result || (retryEmpty && HasTlsCiphertext(*m_tls))) { NetExecution::progress(); }
        m_lastClose = static_cast<int8_t>(result);
    }
    return result;
}

int32_t NetInterfaceTls::status() const
{
    return m_tls ? m_tls->status() : NET_INTERFACE_TLS_MEMORY;
}

int32_t NetInterfaceTls::outputState() const
{
    return m_tls ? m_tls->outputState() : NET_INTERFACE_TLS_MEMORY;
}

int32_t NetInterfaceTls::detail() const
{
    return m_tls ? m_tls->detail() : 0;
}