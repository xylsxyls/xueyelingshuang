#include "ProtoMessageArray.h"
#include "ProtoMessageProtobuf.h"
#include <utility>

ProtoMessageArray::ProtoMessageArray()
{

}

ProtoMessageArray::~ProtoMessageArray()
{

}

ProtoMessageKind ProtoMessageArray::kind() const
{
    return PROTO_MESSAGE_ARRAY;
}

void ProtoMessageArray::clear()
{
    m_values.clear();
}

bool ProtoMessageArray::reserve(size_t values)
{
    if (values > 65535)
    {
        return false;
    }
    try
    {
        m_values.reserve(values);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessageArray::append(const Variant& value)
{
    if (m_values.size() >= 65535 || !ProtoMessageProtobuf::validValue(value))
    {
        return false;
    }
    try
    {
        // 在扩容前复制来自本数组的借用，避免其地址随扩容失效
        Variant copy(value);
        return append(std::move(copy));
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessageArray::append(Variant&& value)
{
    if (m_values.size() >= 65535 || !ProtoMessageProtobuf::validValue(value))
    {
        return false;
    }
    try
    {
        m_values.push_back(std::move(value));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

const Variant* ProtoMessageArray::at(size_t index) const
{
    return index < m_values.size() ? &m_values[index] : nullptr;
}

size_t ProtoMessageArray::size() const
{
    return m_values.size();
}

/** 构建数组，类型检查后自动选择protobuf packed或混合值
@param [in] values 稳定借用的顺序值
@param [out] output 临时protobuf信封
@return 值和预算满足要求
*/
static bool BuildArray(const std::vector<Variant>& values, pt::ProtoSpecializedMessage& output)
{
    return ProtoMessageProtobuf::putValues(values, *output.mutable_array()->mutable_values());
}

bool ProtoMessageArray::from(const void* data, size_t size)
{
    try
    {
        pt::ProtoSpecializedMessage input;
        std::vector<Variant> values;
        if (!ProtoMessageProtobuf::parse(data, size, input) || !ProtoMessageProtobuf::validShape(input) || !input.has_array() ||
            !ProtoMessageProtobuf::getValues(input.array().values(), values))
        {
            clear();
            return false;
        }
        m_values.swap(values);
        return true;
    }
    catch (...)
    {
        clear();
        return false;
    }
}

ProtoMessageArray* ProtoMessageArray::array()
{
    return this;
}

bool ProtoMessageArray::serializedSize(size_t& size) const
{
    size = 0;
    try
    {
        pt::ProtoSpecializedMessage message;
        return BuildArray(m_values, message) && ProtoMessageProtobuf::size(message, size);
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessageArray::serializeTo(void* output, size_t capacity, size_t& written) const
{
    written = 0;
    try
    {
        pt::ProtoSpecializedMessage message;
        return BuildArray(m_values, message) && ProtoMessageProtobuf::write(message, output, capacity, written);
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessageArray::toString(std::string& output) const
{
    try
    {
        pt::ProtoSpecializedMessage message;
        return BuildArray(m_values, message) && ProtoMessageProtobuf::string(message, output);
    }
    catch (...)
    {
        return false;
    }
}