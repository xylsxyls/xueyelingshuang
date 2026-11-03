#include "NetInterfaceMacro.h"
#include "NetInterfaceService.h"
#include <cstring>

/** 固定大端32位读取
@param [in] data 至少4字节
@return 数值
*/
static uint32_t ServiceRead32(const uint8_t* data)
{
    return static_cast<uint32_t>(data[0]) << 24 | static_cast<uint32_t>(data[1]) << 16 | static_cast<uint32_t>(data[2]) << 8 | data[3];
}

/** 固定大端32位写入
@param [out] data 至少4字节
@param [in] value 数值
*/
static void ServiceWrite32(uint8_t* data, uint32_t value)
{
    data[0] = static_cast<uint8_t>(value >> 24);
    data[1] = static_cast<uint8_t>(value >> 16);
    data[2] = static_cast<uint8_t>(value >> 8);
    data[3] = static_cast<uint8_t>(value);
}

/** 检查固定32字节秘密非全零
@param [in] secret 32字节或空
@return 是否有效
*/
static bool ServiceSecretValid(const uint8_t* secret)
{
    if (secret == nullptr)
    {
        return false;
    }
    uint8_t combined = 0;
    for (uint32_t index = 0; index < 32; ++index)
    {
        combined |= secret[index];
    }
    return combined != 0;
}

/** 验证来源，允许完整全零但拒绝半个来源
@param [in] origin 来源
@return 是否一致
*/
static bool ServiceOriginValid(const NetInterfaceOrigin& origin)
{
    uint8_t device = 0;
    for (uint32_t index = 0; index < 16; ++index)
    {
        device |= origin.m_device[index];
    }
    return origin.m_endpoint == 0 ? origin.m_platform == 0 && device == 0 : origin.m_platform >= 1 && origin.m_platform <= 3 && device != 0;
}

int32_t NetInterfaceService::hello(const uint8_t* secret, uint32_t limit, uint8_t* output, uint32_t capacity)
{
    if (!ServiceSecretValid(secret) || limit == 0 || limit > 128 || (output == nullptr && capacity != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (capacity < 48)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    uint8_t bytes[48] = {};
    std::memcpy(bytes, "NIS2", 4);
    ServiceWrite32(bytes + 4, limit);
    std::memcpy(bytes + 8, secret, 32);
    std::memcpy(output, bytes, sizeof(bytes));
    volatile uint8_t* wipe = bytes;
    for (uint32_t index = 0; index < sizeof(bytes); ++index)
    {
        wipe[index] = 0;
    }
    return NI_OK;
}

int32_t NetInterfaceService::authenticate(const NetInterfaceFrame& frame, const uint8_t* secret, uint32_t& limit)
{
    limit = 0;
    if (!ServiceSecretValid(secret) || frame.m_kind != NI_HELLO || frame.m_messageType != 3 || frame.m_requestId != 0 ||
        frame.m_payloadSize != 48 || frame.m_payload == nullptr || std::memcmp(frame.m_payload, "NIS2", 4) != 0)
    {
        return NI_PROTOCOL_ERROR;
    }
    const uint32_t requested = ServiceRead32(frame.m_payload + 4);
    // volatile累加器避免将秘密比较改成遇到首个不同字节即返回。
    volatile uint8_t different = 0;
    for (uint32_t index = 0; index < 32; ++index)
    {
        different |= frame.m_payload[8 + index] ^ secret[index];
    }
    for (uint32_t index = 40; index < 48; ++index)
    {
        different |= frame.m_payload[index];
    }
    if (different != 0 || requested == 0 || requested > 128)
    {
        return NI_PROTOCOL_ERROR;
    }
    limit = requested;
    return NI_OK;
}

int32_t NetInterfaceService::wrap(const NetInterfaceOrigin& origin, const uint8_t* data, uint32_t size, uint8_t* output, uint32_t capacity,
                                  uint32_t& required)
{
    required = 0;
    if (!ServiceOriginValid(origin) || (data == nullptr && size != 0) || (output == nullptr && capacity != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    if (size > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT)
    {
        return NI_LIMIT_EXCEEDED;
    }
    required = size + 32;
    if (capacity < required)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    std::memcpy(output, "NOI2", 4);
    ServiceWrite32(output + 4, origin.m_platform);
    ServiceWrite32(output + 8, static_cast<uint32_t>(origin.m_endpoint >> 32));
    ServiceWrite32(output + 12, static_cast<uint32_t>(origin.m_endpoint));
    std::memcpy(output + 16, origin.m_device, 16);
    if (size != 0)
    {
        std::memcpy(output + 32, data, size);
    }
    return NI_OK;
}

int32_t NetInterfaceService::unwrap(const NetInterfaceFrame& frame, NetInterfaceOrigin& origin, NetInterfaceFrame& business)
{
    const NetInterfaceFrame input = frame;
    origin = NetInterfaceOrigin();
    business = NetInterfaceFrame();
    if ((input.m_kind != NI_REQUEST && input.m_kind != NI_RESPONSE && input.m_kind != NI_CANCEL && input.m_kind != NI_DATA &&
         input.m_kind != NI_DATA_ACK && input.m_kind != NI_EVENT) ||
        input.m_payload == nullptr || input.m_payloadSize < 32 || input.m_payloadSize > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT + 32 ||
        std::memcmp(input.m_payload, "NOI2", 4) != 0)
    {
        return NI_PROTOCOL_ERROR;
    }
    NetInterfaceOrigin parsed = {};
    parsed.m_platform = ServiceRead32(input.m_payload + 4);
    parsed.m_endpoint = static_cast<uint64_t>(ServiceRead32(input.m_payload + 8)) << 32 | ServiceRead32(input.m_payload + 12);
    std::memcpy(parsed.m_device, input.m_payload + 16, 16);
    if (!ServiceOriginValid(parsed) || (input.m_kind == NI_REQUEST && parsed.m_endpoint == 0))
    {
        return NI_PROTOCOL_ERROR;
    }
    origin = parsed;
    business = input;
    business.m_payloadSize -= 32;
    business.m_payload = business.m_payloadSize == 0 ? nullptr : input.m_payload + 32;
    return NI_OK;
}