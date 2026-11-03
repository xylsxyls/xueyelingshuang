#include "NetInterfaceMacro.h"
#include "NetInterface.h"
#include "NetInterfaceCredit.h"
#include "NetInterfaceDirectory.h"
#include "NetInterfaceMessage.h"
#include <cstring>
#include <new>

/** 检查长度并复制输出；不足空间不修改调用方缓冲
@param [in] value 包含NUL的原始字节
@param [out] output 调用方内存，可在capacity为0时为空
@param [in] capacity 容量
@param [out] required 必需字节数，不含额外结束符
@return NetInterfaceResult
*/
static int32_t CopyMessageOutput(const std::string& value, uint8_t* output, uint32_t capacity, uint32_t* required)
{
    *required = static_cast<uint32_t>(value.size());
    if (value.size() > capacity)
    {
        return NI_BUFFER_TOO_SMALL;
    }
    if (!value.empty())
    {
        std::memcpy(output, value.data(), value.size());
    }
    return NI_OK;
}

int32_t NetInterface::messageCreate(NetInterfaceMessageHandle* handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    *handle = nullptr;
    try
    {
        *handle = reinterpret_cast<NetInterfaceMessageHandle>(new NetInterfaceMessage);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

void NetInterface::messageDestroy(NetInterfaceMessageHandle handle)
{
    delete reinterpret_cast<NetInterfaceMessage*>(handle);
}

int32_t NetInterface::messageClear(NetInterfaceMessageHandle handle)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    reinterpret_cast<NetInterfaceMessage*>(handle)->clear();
    return NI_OK;
}

int32_t NetInterface::messageParse(NetInterfaceMessageHandle handle, const uint8_t* input, uint32_t size)
{
    if (handle == nullptr)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceMessage* state = reinterpret_cast<NetInterfaceMessage*>(handle);
    state->clear();
    if (input == nullptr && size != 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (size > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT)
    {
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        std::string bytes;
        if (size != 0)
        {
            bytes.assign(reinterpret_cast<const char*>(input), size);
        }
        if (!state->m_message.from(bytes))
        {
            return NI_PROTOCOL_ERROR;
        }
        state->m_budget = size;
        return NI_OK;
    }
    catch (...)
    {
        state->clear();
        return NI_NO_MEMORY;
    }
}

int32_t NetInterface::messageEncode(NetInterfaceMessageHandle handle, uint8_t* output, uint32_t capacity, uint32_t* required)
{
    if (required != nullptr)
    {
        *required = 0;
    }
    if (handle == nullptr || required == nullptr || (output == nullptr && capacity != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::string bytes;
        if (!reinterpret_cast<NetInterfaceMessage*>(handle)->m_message.toString(bytes))
        {
            return NI_PROTOCOL_ERROR;
        }
        if (bytes.size() > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT)
        {
            return NI_LIMIT_EXCEEDED;
        }
        return CopyMessageOutput(bytes, output, capacity, required);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetInterface::messageSetValue(NetInterfaceMessageHandle handle, int32_t group, int32_t key, int32_t type, const uint8_t* value,
                                    uint32_t size)
{
    if (handle == nullptr || (value == nullptr && size != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceMessage* state = reinterpret_cast<NetInterfaceMessage*>(handle);
    if (size > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT - 64 || state->m_budget > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT - 64 - size)
    {
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        std::string bytes;
        if (size != 0)
        {
            bytes.assign(reinterpret_cast<const char*>(value), size);
        }
        if (!Variant::isValidBlob(bytes, type))
        {
            return NI_INVALID_ARGUMENT;
        }
        state->m_message.setKeyValue(key, Variant(bytes, static_cast<Variant::VariantType>(type), true), group);
        state->m_budget += 64 + size;
        return NI_OK;
    }
    catch (...)
    {
        state->clear();
        return NI_NO_MEMORY;
    }
}

int32_t NetInterface::messageGetValue(NetInterfaceMessageHandle handle, int32_t group, int32_t key, int32_t* type, uint8_t* output,
                                    uint32_t capacity, uint32_t* required)
{
    if (required != nullptr)
    {
        *required = 0;
    }
    if (type != nullptr)
    {
        *type = 0;
    }
    if (handle == nullptr || type == nullptr || required == nullptr || (output == nullptr && capacity != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        Variant value;
        if (!reinterpret_cast<NetInterfaceMessage*>(handle)->m_message.getKeyValue(key, value, group))
        {
            return NI_NOT_FOUND;
        }
        *type = static_cast<int32_t>(value.type());
        return CopyMessageOutput(value.toBlob(), output, capacity, required);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetInterface::messageSetCode(NetInterfaceMessageHandle handle, int32_t code, const uint8_t* error, uint32_t size)
{
    if (handle == nullptr || (error == nullptr && size != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceMessage* state = reinterpret_cast<NetInterfaceMessage*>(handle);
    if (size > 4096 || state->m_budget > NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT - size - 32)
    {
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        state->m_message.setCode(code, size ? std::string(reinterpret_cast<const char*>(error), size) : std::string());
        state->m_budget += size + 32;
        return NI_OK;
    }
    catch (...)
    {
        state->clear();
        return NI_NO_MEMORY;
    }
}

int32_t NetInterface::messageGetCode(NetInterfaceMessageHandle handle, int32_t* code, uint8_t* error, uint32_t capacity, uint32_t* required)
{
    if (required != nullptr)
    {
        *required = 0;
    }
    if (code != nullptr)
    {
        *code = ProtoMessage::PROTO_ERROR;
    }
    if (handle == nullptr || code == nullptr || required == nullptr || (error == nullptr && capacity != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        ProtoMessage& message = reinterpret_cast<NetInterfaceMessage*>(handle)->m_message;
        *code = message.getCode();
        return CopyMessageOutput(message.getErrorMessage(), error, capacity, required);
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}