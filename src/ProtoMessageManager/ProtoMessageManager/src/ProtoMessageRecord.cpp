#include "ProtoMessageRecord.h"
#include "ProtoMessageProtobuf.h"
#include <utility>

ProtoMessageRecord::ProtoMessageRecord()
{

}

ProtoMessageRecord::~ProtoMessageRecord()
{

}

ProtoMessageKind ProtoMessageRecord::kind() const
{
    return PROTO_MESSAGE_RECORD;
}

void ProtoMessageRecord::clear()
{
    m_fields.clear();
}

bool ProtoMessageRecord::reserve(size_t fields)
{
    if (fields > 65535)
    {
        return false;
    }
    try
    {
        m_fields.reserve(fields);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessageRecord::setValue(int32_t key, const Variant& value)
{
    if (!ProtoMessageProtobuf::validValue(value))
    {
        return false;
    }
    try
    {
        Variant copy(value);
        return setValue(key, std::move(copy));
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessageRecord::setValue(int32_t key, Variant&& value)
{
    if (!ProtoMessageProtobuf::validValue(value))
    {
        return false;
    }
    try
    {
        const auto found = m_fields.find(key);
        if (found != m_fields.end())
        {
            found->second = std::move(value);
            return true;
        }
        if (m_fields.size() >= 65535)
        {
            return false;
        }
        m_fields.emplace(key, std::move(value));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessageRecord::setBytes(int32_t key, const void* data, size_t size, Variant::VariantType type)
{
    Variant value;
    return size <= 1048576 && value.assignBytes(data, size, type) && setValue(key, std::move(value));
}

const Variant* ProtoMessageRecord::findValue(int32_t key) const
{
    const auto found = m_fields.find(key);
    return found == m_fields.end() ? nullptr : &found->second;
}

size_t ProtoMessageRecord::size() const
{
    return m_fields.size();
}

/** 构建字段消息，按容器现有顺序填入，不为排序复制字段
@param [in] fields 独占消息的稳定字段
@param [out] output 临时protobuf信封
@return 节点和原始字节预算满足要求
*/
static bool BuildRecord(const std::unordered_map<int32_t, Variant>& fields, pt::ProtoSpecializedMessage& output)
{
    if (fields.size() > 65535)
    {
        return false;
    }
    size_t bytes = 0;
    pt::ProtoMessageRecord* target = output.mutable_record();
    target->mutable_fields()->Reserve(static_cast<int32_t>(fields.size()));
    for (auto field = fields.begin(); field != fields.end(); ++field)
    {
        if (field->second.size() > 1048576 - bytes)
        {
            return false;
        }
        bytes += field->second.size();
        pt::ProtoField* item = target->add_fields();
        item->set_key(field->first);
        if (!ProtoMessageProtobuf::putValue(field->second, *item->mutable_value()))
        {
            return false;
        }
    }
    return true;
}

bool ProtoMessageRecord::from(const void* data, size_t size)
{
    try
    {
        pt::ProtoSpecializedMessage input;
        if (!ProtoMessageProtobuf::parse(data, size, input) || !ProtoMessageProtobuf::validShape(input) || !input.has_record() || input.record().fields_size() > 65535)
        {
            clear();
            return false;
        }
        std::unordered_map<int32_t, Variant> fields;
        fields.reserve(input.record().fields_size());
        size_t probes = 0;
        for (int32_t i = 0; i < input.record().fields_size(); ++i)
        {
            const pt::ProtoField& field = input.record().fields(i);
            Variant value;
            if (!ProtoMessageProtobuf::getValue(field.value(), value))
            {
                clear();
                return false;
            }
            // 外部字段编号可能刻意碰撞，限制累计桶遍历，重复编号同样计入
            const size_t collisions = fields.bucket_size(fields.bucket(field.key()));
            if (collisions > 1048576 - probes)
            {
                clear();
                return false;
            }
            probes += collisions;
            fields[field.key()] = std::move(value);
        }
        m_fields.swap(fields);
        return true;
    }
    catch (...)
    {
        clear();
        return false;
    }
}

ProtoMessageRecord* ProtoMessageRecord::record()
{
    return this;
}

bool ProtoMessageRecord::serializedSize(size_t& size) const
{
    size = 0;
    try
    {
        pt::ProtoSpecializedMessage message;
        return BuildRecord(m_fields, message) && ProtoMessageProtobuf::size(message, size);
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessageRecord::serializeTo(void* output, size_t capacity, size_t& written) const
{
    written = 0;
    try
    {
        pt::ProtoSpecializedMessage message;
        return BuildRecord(m_fields, message) && ProtoMessageProtobuf::write(message, output, capacity, written);
    }
    catch (...)
    {
        return false;
    }
}

bool ProtoMessageRecord::toString(std::string& output) const
{
    try
    {
        pt::ProtoSpecializedMessage message;
        return BuildRecord(m_fields, message) && ProtoMessageProtobuf::string(message, output);
    }
    catch (...)
    {
        return false;
    }
}