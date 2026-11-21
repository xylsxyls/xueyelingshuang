#include "Variant.h"
#include <cstdlib>
#include <clocale>
#include <cstring>
#include <iomanip>
#include <locale>
#include <sstream>
#include <utility>

Variant::Variant() :
m_type(INIT),
m_length(0),
m_storage()
{

}

Variant::Variant(const Variant& value) :
m_type(value.m_type),
m_length(value.m_length),
m_storage(value.m_storage)
{
    // 新对象没有旧资源；内联值直接复制，长字节才建立独占副本
    // new失败时构造未完成，借用的旧指针不会被析构或释放
    if ((m_type == STRING || m_type == BLOB) && m_length > sizeof(m_storage.m_small))
    {
        m_storage.m_heap = new char[m_length];
        std::memcpy(m_storage.m_heap, value.m_storage.m_heap, m_length);
    }
}

Variant::Variant(const std::string& data, VariantType type, bool isBlob) :
Variant()
{
    if (type < INIT || type > BLOB || (isBlob && !isValidBlob(data, static_cast<int32_t>(type))))
    {
        return;
    }
    if (isBlob)
    {
        if (type == INT32)
        {
            int32_t value = 0;
            ::memcpy(&value, data.data(), sizeof(value));
            setInt32(value);
        }
        else if (type == UINT64)
        {
            uint64_t value = 0;
            ::memcpy(&value, data.data(), sizeof(value));
            setUint64(value);
        }
        else if (type == DOUBLE)
        {
            double value = 0;
            ::memcpy(&value, data.data(), sizeof(value));
            setDouble(value);
        }
        else
        {
            setBlob(data);
            m_type = type;
        }
    }
    else if (data.empty())
    {
        // 保持历史空文本数字表示，读取为0但原始序列化长度仍为0。
        m_type = type;
    }
    else
    {
        switch (type)
        {
        case INT32:
        {
            const long number = std::strtol(data.c_str(), nullptr, 10);
            const int32_t bounded = number < INT32_MIN ? INT32_MIN :
                (number > INT32_MAX ? INT32_MAX : static_cast<int32_t>(number));
            setInt32(bounded);
            break;
        }
        case UINT64:
            setUint64(::strtoull(data.c_str(), nullptr, 10));
            break;
        case DOUBLE:
        {
            const char* text = data.c_str();
            char* end = nullptr;
            double value = std::strtod(text, &end);
            // 某些地区使用逗号作小数点。保留已有的本地输入，同时接受 toString 的标准小数点。
            if (*end == '.' || end == text)
            {
                const size_t point = data.find('.');
                if (point != std::string::npos)
                {
                    const char* separator = std::localeconv()->decimal_point;
                    if (std::strcmp(separator, ".") != 0)
                    {
                        std::string localized(data);
                        localized.replace(point, 1, separator);
                        value = std::strtod(localized.c_str(), nullptr);
                    }
                }
            }
            setDouble(value);
            break;
        }
        case STRING:
            setString(data.c_str());
            break;
        case BLOB:
            setBlob(data);
            break;
        default:
            break;
        }
    }
}

bool Variant::isValidBlob(const std::string& data, int32_t type)
{
    return isValidBlob(data.size(), type);
}

bool Variant::isValidBlob(size_t size, int32_t type)
{
    switch (type)
    {
    case INIT:
        return size == 0;
    case INT32:
        return size == sizeof(int32_t);
    case UINT64:
        return size == sizeof(uint64_t);
    case DOUBLE:
        return size == sizeof(double);
    case STRING:
    case BLOB:
        return true;
    default:
        return false;
    }
}

Variant::~Variant()
{
    clear();
}

Variant::Variant(const int32_t& value) :
m_type(INT32),
m_length(sizeof(value)),
m_storage()
{
    std::memcpy(&m_storage.m_scalar, &value, sizeof(value));
}

Variant::Variant(const uint64_t& value) :
m_type(UINT64),
m_length(sizeof(value)),
m_storage()
{
    m_storage.m_scalar = value;
}

Variant::Variant(const double& value) :
m_type(DOUBLE),
m_length(sizeof(value)),
m_storage()
{
    std::memcpy(&m_storage.m_scalar, &value, sizeof(value));
}

Variant::Variant(const char* value) :
Variant()
{
    setString(value);
}

Variant::Variant(const std::string& value) :
Variant()
{
    setBlob(value);
}

Variant& Variant::operator=(const Variant& value)
{
    setVariant(value);
    return *this;
}

Variant& Variant::operator=(const int32_t& value)
{
    setInt32(value);
    return *this;
}

Variant& Variant::operator=(const uint64_t& value)
{
    setUint64(value);
    return *this;
}

Variant& Variant::operator=(const double& value)
{
    setDouble(value);
    return *this;
}

Variant& Variant::operator=(const char* value)
{
    setString(value);
    return *this;
}

Variant& Variant::operator=(const std::string& value)
{
    setBlob(value);
    return *this;
}

const char* Variant::bytes() const
{
    if (m_length == 0)
    {
        return nullptr;
    }
    if (m_type == STRING || m_type == BLOB)
    {
        return m_length <= sizeof(m_storage.m_small) ? m_storage.m_small : m_storage.m_heap;
    }
    return reinterpret_cast<const char*>(&m_storage.m_scalar);
}

bool Variant::operator==(const Variant& value) const
{
    return m_length == value.m_length && (m_length == 0 || std::memcmp(bytes(), value.bytes(), m_length) == 0);
}

bool Variant::operator!=(const Variant& value) const
{
    return !(*this == value);
}

Variant::operator int32_t() const
{
    int32_t value = 0;
    if (m_type == INT32 && m_length == sizeof(value))
    {
        std::memcpy(&value, &m_storage.m_scalar, sizeof(value));
    }
    return value;
}

Variant::operator uint64_t() const
{
    return m_type == UINT64 && m_length == sizeof(m_storage.m_scalar) ? m_storage.m_scalar : 0;
}

Variant::operator double() const
{
    double value = 0;
    if (m_type == DOUBLE && m_length == sizeof(value))
    {
        std::memcpy(&value, &m_storage.m_scalar, sizeof(value));
    }
    return value;
}

Variant::operator std::string() const
{
    return (m_type == STRING || m_type == BLOB) && m_length != 0 ? std::string(bytes(), m_length) : std::string();
}

Variant::VariantType Variant::type() const
{
    return m_type;
}

/** 擦除当前分配块内的有效字节或缩短后不再保留的尾部。
@param [in,out] buffer 待擦除的可写范围起点
@param [in] size 需要归零的字节数
*/
static void EraseBytes(char* buffer, size_t size)
{
    volatile char* output = buffer;
    for (size_t index = 0; index < size; ++index)
    {
        output[index] = 0;
    }
}

void Variant::clearMemory()
{
    if (m_type == STRING || m_type == BLOB)
    {
        char* buffer = m_length <= sizeof(m_storage.m_small) ? m_storage.m_small : m_storage.m_heap;
        EraseBytes(buffer, m_length);
        if (m_length > sizeof(m_storage.m_small))
        {
            delete[] buffer;
        }
    }
    m_length = 0;
    m_storage.m_scalar = 0;
}

void Variant::clear()
{
    clearMemory();
    m_type = INIT;
}

std::string Variant::toString() const
{
    std::string result;
    toString(result);
    return result;
}

std::string Variant::toBlob() const
{
    std::string result;
    toBlob(result);
    return result;
}

void Variant::toString(std::string& result) const
{
    if (m_length == 0 || m_type == STRING || m_type == BLOB)
    {
        toBlob(result);
        return;
    }
    if (m_type == INT32 || m_type == UINT64)
    {
        bool negative = false;
        uint64_t number = m_storage.m_scalar;
        if (m_type == INT32)
        {
            int32_t value = 0;
            std::memcpy(&value, &m_storage.m_scalar, sizeof(value));
            negative = value < 0;
            number = negative ? static_cast<uint64_t>(-static_cast<int64_t>(value)) : static_cast<uint64_t>(value);
        }
        char buffer[32];
        char* end = buffer + sizeof(buffer);
        char* current = end;
        do
        {
            *--current = static_cast<char>('0' + number % 10);
            number /= 10;
        } while (number != 0);
        if (negative)
        {
            *--current = '-';
        }
        result.assign(current, static_cast<size_t>(end - current));
        return;
    }
    // 只有显式转换才构造格式化器，构造Variant不再建立可能抛异常的Debug文本缓存。
    std::ostringstream output;
    output.imbue(std::locale::classic());
    double value = 0;
    std::memcpy(&value, &m_storage.m_scalar, sizeof(value));
    output << std::setprecision(17) << value;
    std::string text = output.str();
    result.swap(text);
}

void Variant::toBlob(std::string& result) const
{
    if (m_length == 0)
    {
        result.clear();
    }
    else
    {
        result.assign(bytes(), m_length);
    }
}

void Variant::setInt32(const int32_t& value)
{
    const int32_t snapshot = value;
    clearMemory();
    std::memcpy(&m_storage.m_scalar, &snapshot, sizeof(snapshot));
    m_length = sizeof(snapshot);
    m_type = INT32;
}

void Variant::setUint64(const uint64_t& value)
{
    const uint64_t snapshot = value;
    clearMemory();
    m_storage.m_scalar = snapshot;
    m_length = sizeof(snapshot);
    m_type = UINT64;
}

void Variant::setDouble(const double& value)
{
    const double snapshot = value;
    clearMemory();
    std::memcpy(&m_storage.m_scalar, &snapshot, sizeof(snapshot));
    m_length = sizeof(snapshot);
    m_type = DOUBLE;
}

void Variant::setString(const char* value)
{
    setBytes(value, value == nullptr ? 0 : std::strlen(value), STRING);
}

void Variant::setBlob(const std::string& value)
{
    setBytes(value.data(), value.size(), BLOB);
}

void Variant::setVariant(const Variant& value)
{
    if (this != &value)
    {
        setBytes(value.bytes(), value.m_length, value.m_type);
    }
}

void Variant::setBytes(const char* data, size_t size, VariantType type)
{
    if ((type == STRING || type == BLOB) && size > sizeof(m_storage.m_small))
    {
        if ((m_type == STRING || m_type == BLOB) && m_length >= size)
        {
            // 借用当前长字节的子区间时源和目标可能重叠。
            std::memmove(m_storage.m_heap, data, size);
            if (size < m_length)
            {
                EraseBytes(m_storage.m_heap + size, m_length - size);
            }
        }
        else
        {
            char* replacement = new char[size];
            std::memcpy(replacement, data, size);
            clearMemory();
            m_storage.m_heap = replacement;
        }
    }
    else
    {
        char replacement[sizeof(m_storage.m_small)];
        if (size != 0)
        {
            // 先复制到局部存储，允许data借用当前值，包括子区间。
            std::memcpy(replacement, data, size);
        }
        clearMemory();
        if (size != 0)
        {
            std::memcpy(&m_storage, replacement, size);
        }
    }
    m_length = size;
    m_type = type;
}

Variant::Variant(Variant&& value) :
m_type(value.m_type),
m_length(value.m_length),
m_storage(value.m_storage)
{
    value.m_type = INIT;
    value.m_length = 0;
    value.m_storage = VariantStorage();
}

Variant& Variant::operator=(Variant&& value)
{
    if (this != &value)
    {
        clearMemory();
        m_type = value.m_type;
        m_length = value.m_length;
        m_storage = value.m_storage;
        value.m_type = INIT;
        value.m_length = 0;
        value.m_storage = VariantStorage();
    }
    return *this;
}

void Variant::swap(Variant& value)
{
    std::swap(m_type, value.m_type);
    std::swap(m_length, value.m_length);
    std::swap(m_storage, value.m_storage);
}

bool Variant::assignBytes(const void* data, size_t size, VariantType type)
{
    if ((data == nullptr && size != 0) || !isValidBlob(size, static_cast<int32_t>(type)))
    {
        return false;
    }
    try
    {
        setBytes(static_cast<const char*>(data), size, type);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

const char* Variant::data() const
{
    return bytes();
}

size_t Variant::size() const
{
    return m_length;
}

// 使用示例开始。单独编译本文件时可启用下方的 main。
//#include <iostream>
//#include <utility>
//
//int main()
//{
//    // 设备采样包含一项数值和可带零字节的负载。保留副本用于记录，再移动原负载交付给下游。
//    Variant temperature(23.5);
//    const std::string bytes("A\0B", 3);
//    Variant packet;
//    if (!packet.assignBytes(bytes.data(), bytes.size(), Variant::BLOB))
//    {
//        return 1;
//    }
//    Variant saved(packet);
//    Variant outbound(std::move(packet));
//    std::string serialized;
//    outbound.toBlob(serialized);
//    if (!Variant::isValidBlob(serialized, outbound.type()) || !outbound.data())
//    {
//        return 1;
//    }
//    std::cout << "temperature " << static_cast<double>(temperature) << ", payload " << outbound.size()
//        << " bytes, first byte " << static_cast<unsigned>(static_cast<unsigned char>(outbound.data()[0])) << '\n';
//
//    Variant record(static_cast<int32_t>(7));
//    std::string text;
//    record.toString(text);
//    std::cout << "record " << text << ", unchanged copy=" << (saved == outbound) << '\n';
//    record = static_cast<uint64_t>(42);
//    std::cout << "sequence " << record.toString() << '\n';
//    record = "sent";
//    std::cout << "state " << record.toString() << '\n';
//    record = bytes;
//    outbound.swap(record);
//    saved.clear();
//    return outbound.toBlob() == bytes ? 0 : 1;
//}
// 使用示例结束。