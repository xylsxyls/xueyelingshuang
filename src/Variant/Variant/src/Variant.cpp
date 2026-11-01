#include "Variant.h"
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <locale>
#include <sstream>

Variant::Variant() :
m_type(INIT),
m_length(0),
m_data(nullptr),
m_scalar(0)
{

}

Variant::Variant(const Variant& value) :
Variant()
{
    setVariant(value);
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
            setInt32(::atoi(data.c_str()));
            break;
        case UINT64:
            setUint64(::strtoull(data.c_str(), nullptr, 10));
            break;
        case DOUBLE:
            setDouble(::atof(data.c_str()));
            break;
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
    switch (type)
    {
    case INIT:
        return data.empty();
    case INT32:
        return data.size() == sizeof(int32_t);
    case UINT64:
        return data.size() == sizeof(uint64_t);
    case DOUBLE:
        return data.size() == sizeof(double);
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
Variant()
{
    setInt32(value);
}

Variant::Variant(const uint64_t& value) :
Variant()
{
    setUint64(value);
}

Variant::Variant(const double& value) :
Variant()
{
    setDouble(value);
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

Variant Variant::operator=(const Variant& value)
{
    setVariant(value);
    return *this;
}

Variant Variant::operator=(const int32_t& value)
{
    setInt32(value);
    return *this;
}

Variant Variant::operator=(const uint64_t& value)
{
    setUint64(value);
    return *this;
}

Variant Variant::operator=(const double& value)
{
    setDouble(value);
    return *this;
}

Variant Variant::operator=(const char* value)
{
    setString(value);
    return *this;
}

Variant Variant::operator=(const std::string& value)
{
    setBlob(value);
    return *this;
}

const char* Variant::bytes() const
{
    return m_length == 0 ? nullptr : (m_type == STRING || m_type == BLOB ? m_data : reinterpret_cast<const char*>(&m_scalar));
}

bool Variant::operator==(const Variant& value)
{
    return m_length == value.m_length && (m_length == 0 || std::memcmp(bytes(), value.bytes(), m_length) == 0);
}

bool Variant::operator!=(const Variant& value)
{
    return !(*this == value);
}

Variant::operator int32_t()
{
    int32_t value = 0;
    if (m_type == INT32 && m_length == sizeof(value))
    {
        std::memcpy(&value, &m_scalar, sizeof(value));
    }
    return value;
}

Variant::operator uint64_t()
{
    return m_type == UINT64 && m_length == sizeof(m_scalar) ? m_scalar : 0;
}

Variant::operator double()
{
    double value = 0;
    if (m_type == DOUBLE && m_length == sizeof(value))
    {
        std::memcpy(&value, &m_scalar, sizeof(value));
    }
    return value;
}

Variant::operator std::string()
{
    return (m_type == STRING || m_type == BLOB) && m_length != 0 ? std::string(m_data, m_length) : std::string();
}

Variant::VariantType Variant::type() const
{
    return m_type;
}

void Variant::clearMemory()
{
    if (m_data != nullptr)
    {
        volatile char* output = m_data;
        for (size_t index = 0; index < m_length; ++index)
        {
            output[index] = 0;
        }
        delete[] m_data;
    }
    m_data = nullptr;
    m_length = 0;
    m_scalar = 0;
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
    // 只有显式转换才构造格式化器，构造Variant不再建立可能抛异常的Debug文本缓存。
    std::ostringstream output;
    output.imbue(std::locale::classic());
    switch (m_type)
    {
    case INT32:
        {
            int32_t value = 0;
            std::memcpy(&value, &m_scalar, sizeof(value));
            output << value;
            break;
        }
    case UINT64:
        {
            output << m_scalar;
            break;
        }
    case DOUBLE:
        {
            double value = 0;
            std::memcpy(&value, &m_scalar, sizeof(value));
            output << std::setprecision(17) << value;
            break;
        }
    default:
        {
            break;
        }
    }
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
    std::memcpy(&m_scalar, &snapshot, sizeof(snapshot));
    m_length = sizeof(snapshot);
    m_type = INT32;
}

void Variant::setUint64(const uint64_t& value)
{
    const uint64_t snapshot = value;
    clearMemory();
    m_scalar = snapshot;
    m_length = sizeof(snapshot);
    m_type = UINT64;
}

void Variant::setDouble(const double& value)
{
    const double snapshot = value;
    clearMemory();
    std::memcpy(&m_scalar, &snapshot, sizeof(snapshot));
    m_length = sizeof(snapshot);
    m_type = DOUBLE;
}

void Variant::setString(const char* value)
{
    const size_t length = value == nullptr ? 0 : std::strlen(value);
    char* data = length == 0 ? nullptr : new char[length];
    if (length != 0)
    {
        std::memcpy(data, value, length);
    }
    clearMemory();
    m_data = data;
    m_length = length;
    m_type = STRING;
}

void Variant::setBlob(const std::string& value)
{
    const size_t length = value.size();
    char* data = length == 0 ? nullptr : new char[length];
    if (length != 0)
    {
        std::memcpy(data, value.data(), length);
    }
    clearMemory();
    m_data = data;
    m_length = length;
    m_type = BLOB;
}

void Variant::setVariant(const Variant& value)
{
    if (this == &value)
    {
        return;
    }
    char* data = nullptr;
    if ((value.m_type == STRING || value.m_type == BLOB) && value.m_length != 0)
    {
        data = new char[value.m_length];
        std::memcpy(data, value.m_data, value.m_length);
    }
    clearMemory();
    m_data = data;
    m_scalar = value.m_scalar;
    m_length = value.m_length;
    m_type = value.m_type;
}

// VARIANT_OWNERSHIP_UNIT_BEGIN
//#include <cstdio>
//#include <cstring>
//#include <limits>
//#ifdef _DEBUG
//#include <crtdbg.h>
//#endif
//
///** 独立启用；数值原始位、字节深拷贝、空表示和分配失败路径分别核对
//@return 0全部通过，1失败；DLL消费不跨模块转交std::string释放权
//*/
//int main()
//{
//    uint32_t checks = 0;
//    // condition为期望结果，message为静态失败说明；异常仅由本main捕获。
//    const auto Check = [&checks](bool condition, const char* message)
//    {
//        ++checks;
//        if (!condition)
//        {
//            std::printf("FAIL %s\n", message);
//            throw 1;
//        }
//    };
//    try
//    {
//        const int32_t signedValues[] = {0, -1, INT32_MIN, INT32_MAX, 1234567};
//        const uint64_t unsignedValues[] = {0, 1, UINT64_MAX, UINT64_C(0xfedcba9876543210)};
//        for (uint32_t pass = 0; pass < 1000; ++pass)
//        {
//            for (size_t index = 0; index < sizeof(signedValues) / sizeof(signedValues[0]); ++index)
//            {
//                Variant value(signedValues[index]);
//                Variant copied(value);
//                value.clear();
//                Check(value.type() == Variant::INIT && copied.type() == Variant::INT32 && static_cast<int32_t>(copied) == signedValues[index],
//                    "signed scalar copy or cleanup");
//                copied = copied;
//                Check(static_cast<int32_t>(copied) == signedValues[index] && static_cast<uint64_t>(copied) == 0, "self assignment or wrong-type read");
//#ifdef STATIC_LIB
//                const std::string raw(reinterpret_cast<const char*>(&signedValues[index]), sizeof(int32_t));
//                Check(copied.toBlob() == raw, "signed native bytes changed");
//                Variant decoded(raw, Variant::INT32, true);
//                Check(static_cast<int32_t>(decoded) == signedValues[index], "signed blob round trip");
//#endif
//            }
//            for (size_t index = 0; index < sizeof(unsignedValues) / sizeof(unsignedValues[0]); ++index)
//            {
//                Variant value(unsignedValues[index]);
//                Variant assigned;
//                Variant returned = assigned = value;
//                value.clear();
//                Check(static_cast<uint64_t>(assigned) == unsignedValues[index] && returned == assigned,
//                    "unsigned assignment snapshot");
//                assigned = static_cast<int32_t>(-7);
//                Check(static_cast<uint64_t>(returned) == unsignedValues[index] && static_cast<int32_t>(assigned) == -7,
//                    "assignment return value must remain independent");
//            }
//        }
//        const uint64_t floatBits[] = {0, UINT64_C(0x8000000000000000), UINT64_C(0x3ff4000000000000),
//            UINT64_C(0x7ff0000000000000), UINT64_C(0x7ff8000000001234)};
//        for (size_t index = 0; index < sizeof(floatBits) / sizeof(floatBits[0]); ++index)
//        {
//            double number = 0;
//            std::memcpy(&number, &floatBits[index], sizeof(number));
//            Variant value(number);
//            Variant copy(value);
//            double actual = static_cast<double>(copy);
//            uint64_t bits = 0;
//            std::memcpy(&bits, &actual, sizeof(bits));
//            Check(bits == floatBits[index], "double bit representation changed");
//        }
//        const std::string bytes("A\0B\xff", 4);
//        Variant blob(bytes);
//        // 文本入口按首个零字节截断，显式长度入口和原始反序列化按完整长度保存。
//        Variant ordinaryText(bytes.c_str());
//        Variant explicitText(bytes, Variant::STRING, false);
//        Variant explicitBlob(bytes, Variant::BLOB, false);
//        Variant assignedText;
//        assignedText = bytes.c_str();
//        Variant assignedBlob;
//        assignedBlob = bytes;
//        const Variant expectedText("A");
//        Check(ordinaryText.type() == Variant::STRING && ordinaryText == expectedText &&
//            explicitText.type() == Variant::STRING && explicitText == expectedText &&
//            assignedText.type() == Variant::STRING && assignedText == expectedText, "text stops at first NUL");
//        Check(explicitBlob.type() == Variant::BLOB && explicitBlob == blob &&
//            assignedBlob.type() == Variant::BLOB && assignedBlob == blob, "blob preserves embedded NUL");
//        Variant zeroFirst(std::string("\0A", 2), Variant::STRING, false);
//        Variant zeroBlob(std::string("\0A", 2));
//        Check(zeroFirst.type() == Variant::STRING && zeroFirst == Variant("") &&
//            zeroBlob.type() == Variant::BLOB && zeroBlob != zeroFirst, "leading NUL text and blob distinction");
//        Variant text(bytes, Variant::STRING, true);
//        Check(blob == text && blob.type() == Variant::BLOB && text.type() == Variant::STRING, "historical byte-only comparison changed");
//        Variant textCopy(text);
//        Variant textAssigned;
//        textAssigned = text;
//        Check(textCopy.type() == Variant::STRING && textCopy == blob &&
//            textAssigned.type() == Variant::STRING && textAssigned == blob &&
//            Variant::isValidBlob(bytes, Variant::STRING), "serialized STRING preserves bytes and type");
//        Variant copy(blob);
//        blob.clear();
//        Check(copy == text && copy != blob, "byte ownership not independent");
//        Variant empty;
//        Variant emptyText(static_cast<const char*>(nullptr));
//        Check(empty == emptyText && emptyText.type() == Variant::STRING, "empty value comparison changed");
//        for (int32_t type = -1; type <= 7; ++type)
//        {
//            for (uint32_t length = 0; length <= 9; ++length)
//            {
//                const std::string data(length, 'x');
//                const bool valid = (type == 0 && length == 0) || (type == 1 && length == 4) ||
//                    ((type == 2 || type == 3) && length == 8) || type == 4 || type == 5;
//                Check(Variant::isValidBlob(data, type) == valid, "variant width validation");
//                Variant value(data, static_cast<Variant::VariantType>(type), true);
//                Check(value.type() == (valid ? static_cast<Variant::VariantType>(type) : Variant::INIT), "invalid variant constructed");
//            }
//        }
//#ifdef STATIC_LIB
//        Check(copy.toBlob() == bytes && static_cast<std::string>(text) == bytes && text.toString() == bytes, "binary/text output truncation");
//        Variant negative(static_cast<int32_t>(-123));
//        Variant maximum(static_cast<uint64_t>(UINT64_MAX));
//        Variant fraction(1.25);
//        Check(negative.toString() == "-123" && maximum.toString() == "18446744073709551615" && fraction.toString() == "1.25",
//            "numeric text compatibility");
//        Variant emptyNumber(std::string(), Variant::UINT64, false);
//        Check(emptyNumber.type() == Variant::UINT64 && emptyNumber.toBlob().empty() && static_cast<uint64_t>(emptyNumber) == 0,
//            "legacy empty numeric text changed");
//#endif
//#ifdef _DEBUG
//        static uint32_t s_seen = 0;
//        static uint32_t s_failAt = 0;
//        // 标准CRT钩子参数；只拒绝第s_failAt项普通分配，0只计数。
//        const auto Hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
//        {
//            if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK)
//            {
//                return ++s_seen == s_failAt ? 0 : 1;
//            }
//            return 1;
//        };
//        s_seen = 0;
//        _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(Hook);
//        for (uint32_t index = 0; index < 10000; ++index)
//        {
//            Variant first(static_cast<uint64_t>(index));
//            Variant second(first);
//            second = static_cast<int32_t>(index);
//        }
//        _CrtSetAllocHook(previous);
//        Check(s_seen == 0, "scalar construction/copy/assignment allocated");
//        const std::string payload(100, 'x');
//        _CrtMemState before;
//        _CrtMemState after;
//        _CrtMemCheckpoint(&before);
//        uint32_t rejected = 0;
//        for (s_failAt = 1; s_failAt <= 16; ++s_failAt)
//        {
//            Variant original(static_cast<int32_t>(17));
//            s_seen = 0;
//            previous = _CrtSetAllocHook(Hook);
//            try
//            {
//                Variant value(payload);
//                Variant copied(value);
//                original = copied;
//            }
//            catch (...)
//            {
//                ++rejected;
//            }
//            _CrtSetAllocHook(previous);
//            Check(original.type() == Variant::INT32 || original.type() == Variant::BLOB, "failed assignment corrupted type");
//            original = static_cast<uint64_t>(19);
//            Check(static_cast<uint64_t>(original) == 19, "assignment did not recover after failure");
//        }
//        _CrtMemCheckpoint(&after);
//        Check(rejected != 0 && before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] &&
//            before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK], "variant construction/assignment leaked on failure");
//#endif
//        std::printf("Variant ownership checks=%u PASS\n", checks);
//        return 0;
//    }
//    catch (...)
//    {
//        std::printf("Variant ownership checks=%u FAIL\n", checks);
//        return 1;
//    }
//}
// VARIANT_OWNERSHIP_UNIT_END