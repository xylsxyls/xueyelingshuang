#include "NetInterfaceTls.h"
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

NetInterfaceTls::NetInterfaceTls(const NetInterfaceTlsOptions& options) :
m_tls(nullptr)
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
    m_tls = CEncodeDecode::createTls(crypto);
}

NetInterfaceTls::~NetInterfaceTls()
{
    CEncodeDecode::destroyTls(m_tls);
}

int32_t NetInterfaceTls::feed(const uint8_t* data, uint32_t size, uint32_t& consumed)
{
    consumed = 0;
    return m_tls ? m_tls->feed(data, size, consumed) : NET_INTERFACE_TLS_MEMORY;
}

int32_t NetInterfaceTls::progress(uint64_t monotonicMs)
{
    return m_tls ? m_tls->progress(monotonicMs) : NET_INTERFACE_TLS_MEMORY;
}

int32_t NetInterfaceTls::peekOutput(const uint8_t*& data, uint32_t& size) const
{
    data = nullptr;
    size = 0;
    return m_tls ? m_tls->peekOutput(data, size) : NET_INTERFACE_TLS_MEMORY;
}

int32_t NetInterfaceTls::consumeOutput(uint32_t size)
{
    return m_tls ? m_tls->consumeOutput(size) : NET_INTERFACE_TLS_MEMORY;
}

int32_t NetInterfaceTls::write(const uint8_t* data, uint32_t size, uint32_t& accepted)
{
    accepted = 0;
    return m_tls ? m_tls->write(data, size, accepted) : NET_INTERFACE_TLS_MEMORY;
}

int32_t NetInterfaceTls::append(const uint8_t* data, uint32_t size, uint32_t& accepted)
{
    accepted = 0;
    return m_tls ? m_tls->append(data, size, accepted) : NET_INTERFACE_TLS_MEMORY;
}

int32_t NetInterfaceTls::appendExact(const uint8_t* data, uint32_t size)
{
    return m_tls ? m_tls->appendExact(data, size) : NET_INTERFACE_TLS_MEMORY;
}

int32_t NetInterfaceTls::appendExact(const uint8_t* first, uint32_t firstSize, const uint8_t* second, uint32_t secondSize)
{
    return m_tls ? m_tls->appendExact(first, firstSize, second, secondSize) : NET_INTERFACE_TLS_MEMORY;
}

int32_t NetInterfaceTls::read(uint8_t* output, uint32_t capacity, uint32_t& received)
{
    received = 0;
    return m_tls ? m_tls->read(output, capacity, received) : NET_INTERFACE_TLS_MEMORY;
}

int32_t NetInterfaceTls::finishInput()
{
    return m_tls ? m_tls->finishInput() : NET_INTERFACE_TLS_MEMORY;
}

int32_t NetInterfaceTls::close()
{
    return m_tls ? m_tls->close() : NET_INTERFACE_TLS_MEMORY;
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