#include "ProtoMessageProtobuf.h"
#include <google/protobuf/io/coded_stream.h>
#include <cstring>

bool ProtoMessageProtobuf::validValue(const Variant& value)
{
    return value.size() <= 1048576 && Variant::isValidBlob(value.size(), value.type());
}

bool ProtoMessageProtobuf::putValue(const Variant& value, pt::ProtoValue& output)
{
    if (!validValue(value))
    {
        return false;
    }
    switch (value.type())
    {
    case Variant::INIT:
        output.Clear();
        break;
    case Variant::INT32:
        output.set_signedvalue(static_cast<int32_t>(value));
        break;
    case Variant::UINT64:
        output.set_unsignedvalue(static_cast<uint64_t>(value));
        break;
    case Variant::DOUBLE:
    {
        uint64_t bits = 0;
        std::memcpy(&bits, value.data(), sizeof(bits));
        output.set_doublebits(bits);
        break;
    }
    case Variant::STRING:
        output.mutable_stringvalue()->assign(value.size() == 0 ? "" : value.data(), value.size());
        break;
    case Variant::BLOB:
        output.mutable_blobvalue()->assign(value.size() == 0 ? "" : value.data(), value.size());
        break;
    default:
        return false;
    }
    return true;
}

bool ProtoMessageProtobuf::getValue(const pt::ProtoValue& input, Variant& value)
{
    const int32_t count = input.has_signedvalue() + input.has_unsignedvalue() + input.has_doublebits() +
        input.has_stringvalue() + input.has_blobvalue();
    if (count > 1)
    {
        return false;
    }
    if (input.has_signedvalue())
    {
        value = Variant(static_cast<int32_t>(input.signedvalue()));
    }
    else if (input.has_unsignedvalue())
    {
        value = Variant(static_cast<uint64_t>(input.unsignedvalue()));
    }
    else if (input.has_doublebits())
    {
        const uint64_t bits = input.doublebits();
        return value.assignBytes(&bits, sizeof(bits), Variant::DOUBLE);
    }
    else if (input.has_stringvalue())
    {
        return value.assignBytes(input.stringvalue().data(), input.stringvalue().size(), Variant::STRING);
    }
    else if (input.has_blobvalue())
    {
        return value.assignBytes(input.blobvalue().data(), input.blobvalue().size(), Variant::BLOB);
    }
    else
    {
        value.clear();
    }
    return true;
}

bool ProtoMessageProtobuf::validShape(const pt::ProtoSpecializedMessage& input)
{
    return input.has_record() + input.has_array() + input.has_table() == 1;
}

bool ProtoMessageProtobuf::putValues(const std::vector<Variant>& values, pt::ProtoValues& output)
{
    if (values.size() > 65535)
    {
        return false;
    }
    size_t bytes = 0;
    int32_t common = values.empty() ? -1 : values.front().type();
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (!validValue(values[i]) || values[i].size() > 1048576 - bytes)
        {
            return false;
        }
        bytes += values[i].size();
        if (common != values[i].type())
        {
            common = -1;
        }
    }
    if (common == Variant::INIT)
    {
        output.mutable_emptyvalues()->set_count(static_cast<uint32_t>(values.size()));
        return true;
    }
    if (common == Variant::INT32)
    {
        pt::ProtoSignedValues* target = output.mutable_signedvalues();
        target->mutable_values()->Reserve(static_cast<int32_t>(values.size()));
        for (size_t i = 0; i < values.size(); ++i)
        {
            target->add_values(static_cast<int32_t>(values[i]));
        }
    }
    else if (common == Variant::UINT64)
    {
        pt::ProtoUnsignedValues* target = output.mutable_unsignedvalues();
        target->mutable_values()->Reserve(static_cast<int32_t>(values.size()));
        for (size_t i = 0; i < values.size(); ++i)
        {
            target->add_values(static_cast<uint64_t>(values[i]));
        }
    }
    else if (common == Variant::DOUBLE)
    {
        pt::ProtoDoubleValues* target = output.mutable_doublevalues();
        target->mutable_values()->Reserve(static_cast<int32_t>(values.size()));
        for (size_t i = 0; i < values.size(); ++i)
        {
            uint64_t bits = 0;
            std::memcpy(&bits, values[i].data(), sizeof(bits));
            target->add_values(bits);
        }
    }
    else
    {
        pt::ProtoMixedValues* target = output.mutable_mixed();
        target->mutable_values()->Reserve(static_cast<int32_t>(values.size()));
        for (size_t i = 0; i < values.size(); ++i)
        {
            if (!putValue(values[i], *target->add_values()))
            {
                return false;
            }
        }
    }
    return true;
}

bool ProtoMessageProtobuf::getValues(const pt::ProtoValues& input, std::vector<Variant>& values)
{
    if (input.has_mixed() + input.has_signedvalues() + input.has_unsignedvalues() +
        input.has_doublevalues() + input.has_emptyvalues() != 1)
    {
        return false;
    }
    const size_t count = input.has_mixed() ? input.mixed().values_size() :
        input.has_signedvalues() ? input.signedvalues().values_size() :
        input.has_unsignedvalues() ? input.unsignedvalues().values_size() :
        input.has_doublevalues() ? input.doublevalues().values_size() : input.emptyvalues().count();
    if (count > 65535)
    {
        return false;
    }
    values.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
        const int32_t index = static_cast<int32_t>(i);
        if (input.has_mixed())
        {
            if (!getValue(input.mixed().values(index), values[i]))
            {
                return false;
            }
        }
        else if (input.has_signedvalues())
        {
            values[i] = Variant(static_cast<int32_t>(input.signedvalues().values(index)));
        }
        else if (input.has_unsignedvalues())
        {
            values[i] = Variant(static_cast<uint64_t>(input.unsignedvalues().values(index)));
        }
        else if (input.has_doublevalues())
        {
            const uint64_t bits = input.doublevalues().values(index);
            if (!values[i].assignBytes(&bits, sizeof(bits), Variant::DOUBLE))
            {
                return false;
            }
        }
    }
    return true;
}

bool ProtoMessageProtobuf::parse(const void* data, size_t size, google::protobuf::MessageLite& message)
{
    if (size > 1048576 || (data == nullptr && size != 0))
    {
        return false;
    }
    const uint8_t empty = 0;
    google::protobuf::io::CodedInputStream stream(size == 0 ? &empty : static_cast<const uint8_t*>(data), static_cast<int32_t>(size));
    stream.SetTotalBytesLimit(1048576, 1048576);
    stream.SetRecursionLimit(16);
    return message.ParseFromCodedStream(&stream) && stream.ConsumedEntireMessage();
}

bool ProtoMessageProtobuf::size(const google::protobuf::MessageLite& message, size_t& size)
{
    size = message.ByteSizeLong();
    if (size > 1048576)
    {
        size = 0;
        return false;
    }
    return true;
}

bool ProtoMessageProtobuf::write(const google::protobuf::MessageLite& message, void* output, size_t capacity, size_t& written)
{
    written = 0;
    size_t required = 0;
    if (!size(message, required) || capacity < required || (output == nullptr && required != 0))
    {
        return false;
    }
    uint8_t empty = 0;
    if (!message.SerializeToArray(required == 0 ? &empty : output, static_cast<int32_t>(required)))
    {
        return false;
    }
    written = required;
    return true;
}

bool ProtoMessageProtobuf::string(const google::protobuf::MessageLite& message, std::string& output)
{
    size_t required = 0;
    if (!size(message, required))
    {
        return false;
    }
    std::string result;
    if (!message.SerializeToString(&result))
    {
        return false;
    }
    output.swap(result);
    return true;
}