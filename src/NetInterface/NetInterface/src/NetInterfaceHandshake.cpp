#include "NetInterfaceMacro.h"
#include "NetInterfaceHandshake.h"
#include <algorithm>
#include <cstring>

/** 从固定握手体读取大端uint32
@param [in] bytes 至少4字节
@return 无未对齐访问的整数值
*/
static uint32_t ReadHandshakeWord(const uint8_t* bytes)
{
    return (static_cast<uint32_t>(bytes[0]) << 24) | (static_cast<uint32_t>(bytes[1]) << 16) | (static_cast<uint32_t>(bytes[2]) << 8) |
           bytes[3];
}

/** 写大端uint32
@param [out] bytes 至少4字节
@param [in] value 待写整数
*/
static void WriteHandshakeWord(uint8_t* bytes, uint32_t value)
{
    bytes[0] = static_cast<uint8_t>(value >> 24);
    bytes[1] = static_cast<uint8_t>(value >> 16);
    bytes[2] = static_cast<uint8_t>(value >> 8);
    bytes[3] = static_cast<uint8_t>(value);
}

/** 判断16字节标识不是全零，不用于认证秘密比较
@param [in] bytes 固定16字节
@return 至少一位非零
*/
static bool HasHandshakeIdentity(const uint8_t* bytes)
{
    uint8_t combined = 0;
    for (uint32_t index = 0; index < 16; ++index)
    {
        combined |= bytes[index];
    }
    return combined != 0;
}

NetInterfaceHandshake::NetInterfaceHandshake(const NetInterfaceHandshakeOptions& options, uint64_t nowMs) :
m_options(options),
m_lastMs(nowMs),
m_deadlineMs(0),
m_outputKind(0),
m_status(NI_NEED_MORE)
{
    std::memset(m_peerNonce, 0, sizeof(m_peerNonce));
    std::memset(m_clientDevice, 0, sizeof(m_clientDevice));
    std::memset(m_agreed, 0, sizeof(m_agreed));
    std::memset(m_output, 0, sizeof(m_output));
    if (options.m_server > 1 || options.m_capabilities > 15 || (options.m_capabilities & 1) == 0 || options.m_maxRequests == 0 ||
        options.m_maxRequests > 128 || options.m_maxChunk < 1024 || options.m_maxChunk > 65536 || options.m_timeoutMs < 100 ||
        options.m_timeoutMs > 60000 || nowMs > UINT64_MAX - options.m_timeoutMs || !HasHandshakeIdentity(options.m_nonce) ||
        (options.m_server != 0 && (options.m_platform != 0 || HasHandshakeIdentity(options.m_device))) ||
        (options.m_server == 0 && (options.m_platform < 1 || options.m_platform > 3 || !HasHandshakeIdentity(options.m_device))))
    {
        fail(NI_INVALID_ARGUMENT);
        return;
    }
    m_deadlineMs = nowMs + options.m_timeoutMs;
    if (options.m_server == 0)
    {
        m_agreed[0] = options.m_platform;
        m_agreed[1] = options.m_capabilities;
        m_agreed[2] = options.m_maxRequests;
        m_agreed[3] = options.m_maxChunk;
        std::memcpy(m_clientDevice, options.m_device, 16);
        makeOutput(false);
    }
}

int32_t NetInterfaceHandshake::fail(int32_t result)
{
    m_outputKind = 0;
    m_status = result;
    return result;
}

int32_t NetInterfaceHandshake::progress(uint64_t nowMs)
{
    if (m_status < 0)
    {
        return m_status;
    }
    if (nowMs < m_lastMs)
    {
        return fail(NI_INVALID_ARGUMENT);
    }
    m_lastMs = nowMs;
    if (m_status != NI_READY && nowMs >= m_deadlineMs)
    {
        return fail(NI_TIMEOUT);
    }
    return m_status;
}

void NetInterfaceHandshake::makeOutput(bool reply)
{
    std::memset(m_output, 0, sizeof(m_output));
    WriteHandshakeWord(m_output, reply ? 0x4E485232U : 0x4E484132U);
    WriteHandshakeWord(m_output + 4, 2);
    WriteHandshakeWord(m_output + 8, m_agreed[1]);
    WriteHandshakeWord(m_output + 12, m_agreed[0]);
    WriteHandshakeWord(m_output + 16, m_agreed[2]);
    WriteHandshakeWord(m_output + 20, m_agreed[3]);
    std::memcpy(m_output + 24, reply ? m_peerNonce : m_options.m_nonce, 16);
    std::memcpy(m_output + 40, reply ? m_options.m_nonce : m_clientDevice, 16);
    m_outputKind = static_cast<uint16_t>(reply ? NI_HELLO_REPLY : NI_HELLO);
}

int32_t NetInterfaceHandshake::accept(const NetInterfaceFrame& frame, uint64_t nowMs)
{
    if (progress(nowMs) < 0)
    {
        return m_status;
    }
    if (m_status == NI_READY)
    {
        if (frame.m_kind < NI_REQUEST || frame.m_kind > NI_DATA_ACK)
        {
            return fail(NI_PROTOCOL_ERROR);
        }
        return NI_OK;
    }
    if (m_outputKind != 0 || frame.m_kind != (m_options.m_server ? NI_HELLO : NI_HELLO_REPLY) || frame.m_productId != 0 ||
        frame.m_requestId != 0 || frame.m_messageType != 2 || frame.m_payloadSize != 64 || frame.m_payload == nullptr)
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    const uint8_t* body = frame.m_payload;
    const uint32_t expectedMagic = m_options.m_server ? 0x4E484132U : 0x4E485232U;
    const uint32_t capabilities = ReadHandshakeWord(body + 8);
    const uint32_t platform = ReadHandshakeWord(body + 12);
    const uint32_t requests = ReadHandshakeWord(body + 16);
    const uint32_t chunk = ReadHandshakeWord(body + 20);
    if (ReadHandshakeWord(body) != expectedMagic || ReadHandshakeWord(body + 4) != 2 || capabilities > 15 || (capabilities & 1) == 0 ||
        platform < 1 || platform > 3 || requests == 0 || requests > 128 || chunk < 1024 || chunk > 65536 ||
        ReadHandshakeWord(body + 56) != 0 || ReadHandshakeWord(body + 60) != 0 || !HasHandshakeIdentity(body + 24) ||
        !HasHandshakeIdentity(body + 40))
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    if (m_options.m_server == 0 &&
        (platform != m_options.m_platform || (capabilities & ~m_options.m_capabilities) != 0 || requests > m_options.m_maxRequests ||
         chunk > m_options.m_maxChunk || std::memcmp(body + 24, m_options.m_nonce, 16) != 0))
    {
        return fail(NI_PROTOCOL_ERROR);
    }
    m_agreed[0] = platform;
    m_agreed[1] = capabilities & m_options.m_capabilities;
    m_agreed[2] = (std::min)(requests, m_options.m_maxRequests);
    m_agreed[3] = (std::min)(chunk, m_options.m_maxChunk);
    if (m_options.m_server != 0)
    {
        std::memcpy(m_peerNonce, body + 24, 16);
        std::memcpy(m_clientDevice, body + 40, 16);
        makeOutput(true);
    }
    else
    {
        std::memcpy(m_peerNonce, body + 40, 16);
        m_status = NI_READY;
    }
    return m_status;
}

int32_t NetInterfaceHandshake::peekOutput(NetInterfaceFrame& frame) const
{
    std::memset(&frame, 0, sizeof(frame));
    if (m_status < 0)
    {
        return m_status;
    }
    if (m_outputKind == 0)
    {
        return NI_NEED_MORE;
    }
    frame.m_kind = m_outputKind;
    frame.m_messageType = 2;
    frame.m_payload = m_output;
    frame.m_payloadSize = sizeof(m_output);
    return NI_READY;
}

int32_t NetInterfaceHandshake::consumeOutput()
{
    if (m_status < 0)
    {
        return m_status;
    }
    if (m_outputKind == 0)
    {
        return NI_INVALID_STATE;
    }
    m_outputKind = 0;
    if (m_options.m_server != 0)
    {
        m_status = NI_READY;
    }
    return m_status;
}

int32_t NetInterfaceHandshake::peerInfo(uint32_t fields[4], uint8_t peerNonce[16], uint8_t clientDevice[16]) const
{
    if (fields == nullptr || peerNonce == nullptr || clientDevice == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    std::memset(fields, 0, 4 * sizeof(uint32_t));
    std::memset(peerNonce, 0, 16);
    std::memset(clientDevice, 0, 16);
    if (m_status == NI_READY)
    {
        std::memcpy(fields, m_agreed, sizeof(m_agreed));
        std::memcpy(peerNonce, m_peerNonce, 16);
        std::memcpy(clientDevice, m_clientDevice, 16);
    }
    return m_status;
}