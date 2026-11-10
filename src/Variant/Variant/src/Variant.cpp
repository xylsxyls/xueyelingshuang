#include "Variant.h"
#include <cstdlib>
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

void Variant::clearMemory()
{
    if (m_type == STRING || m_type == BLOB)
    {
        char* buffer = m_length <= sizeof(m_storage.m_small) ? m_storage.m_small : m_storage.m_heap;
        volatile char* output = buffer;
        for (size_t index = 0; index < m_length; ++index)
        {
            output[index] = 0;
        }
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
        char* replacement = new char[size];
        std::memcpy(replacement, data, size);
        clearMemory();
        m_storage.m_heap = replacement;
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

// VARIANT_PERF_UNIT_BEGIN
//#include <cstdlib>
//#include <algorithm>
//#include <chrono>
//#include <cstdio>
//#include <cstring>
//#include <locale>
//#include <sstream>
//#include <utility>
//#include <vector>
//#if defined(_MSC_VER) && defined(_DEBUG)
//#include <crtdbg.h>
//#endif
//
///** 独立正确性／性能入口；正式库保持注释，测试副本单独启用
//@param [in] argc 参数数量
//@param [in] argv bench表示同机基准，其他情况执行边界用例；NEW_VARIANT_TEST仅供旧版对比裁剪新接口
//@return 0通过，1失败；性能输出为每次操作纳秒，不代表网络吞吐
//*/
//int main(int argc, char** argv)
//{
//    uint32_t checks = 0;
//    uint32_t failures = 0;
//    // condition为独立预期，reason为静态说明；仅记录，不终止产品进程。
//    const auto Check = [&checks, &failures](bool condition, const char* reason)
//    {
//        ++checks;
//        if (!condition)
//        {
//            ++failures;
//            std::printf("FAIL %s\n", reason);
//        }
//    };
//    try
//    {
//        if (argc > 1 && std::strcmp(argv[1], "bench") == 0)
//        {
//            const char* names[] = {"scalar_construct", "scalar_assign", "blob16_assign", "blob64_assign", "blob4096_assign",
//                "blob16_copy", "blob64_copy", "integer_text", "blob4096_copy_read", "blob4096_move_chain", "raw64_ingest", "blob4096_borrow"};
//            std::string payloads[] = {std::string(16, 'x'), std::string(64, 'y'), std::string(4096, 'z')};
//            Variant source16(payloads[0]);
//            Variant source64(payloads[1]);
//            Variant source4096(payloads[2]);
//            Variant target;
//            volatile uint64_t checksum = 0;
//            for (uint32_t scenario = 0; scenario < 12; ++scenario)
//            {
//                if (argc > 2 && scenario != static_cast<uint32_t>(std::strtoul(argv[2], nullptr, 10)))
//                {
//                    continue;
//                }
//                size_t iterations = 2048;
//                double seconds = 0;
//                do
//                {
//                    const auto started = std::chrono::steady_clock::now();
//                    for (size_t i = 0; i < iterations; ++i)
//                    {
//                        switch (scenario)
//                        {
//                        case 0:
//                            {
//                                Variant value(static_cast<int32_t>(i));
//                                checksum += static_cast<int32_t>(value);
//                                break;
//                            }
//                        case 1:
//                            target = static_cast<int32_t>(i);
//                            checksum += static_cast<int32_t>(target);
//                            break;
//                        case 2:
//                        case 3:
//                        case 4:
//                            target = payloads[scenario - 2];
//                            checksum += target.type();
//                            break;
//                        case 5:
//                            {
//                                Variant value(source16);
//                                checksum += value.type();
//                                break;
//                            }
//                        case 6:
//                            {
//                                Variant value(source64);
//                                checksum += value.type();
//                                break;
//                            }
//                        case 7:
//                            {
//                                Variant value(static_cast<int32_t>(-1234567));
//                                checksum += value.toString().size();
//                                break;
//                            }
//                        case 8:
//                            checksum += source4096.toBlob().size();
//                            break;
//                        case 9:
//                            {
//                                Variant first(payloads[2]);
//                                Variant second(std::move(first));
//                                Variant third(std::move(second));
//                                checksum += third.type();
//                                break;
//                            }
//                        case 10:
//                            {
//#if NEW_VARIANT_TEST
//                                if (!target.assignBytes(payloads[1].data(), payloads[1].size()))
//                                {
//                                    return 2;
//                                }
//#else
//                                target = std::string(payloads[1].data(), payloads[1].size());
//#endif
//                                checksum += target.type();
//                                break;
//                            }
//                        case 11:
//                            {
//#if NEW_VARIANT_TEST
//                                const char* bytes = source4096.data();
//                                checksum += source4096.size() + static_cast<unsigned char>(bytes[0]);
//#else
//                                const std::string bytes = source4096.toBlob();
//                                checksum += bytes.size() + static_cast<unsigned char>(bytes[0]);
//#endif
//                                break;
//                            }
//                        }
//                    }
//                    seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
//                    if (seconds >= 0.04 || iterations >= 134217728)
//                    {
//                        break;
//                    }
//                    else
//                    {
//                        iterations *= 2;
//                    }
//                } while (true);
//                std::printf("BENCH,%s,%llu,%.4f\n", names[scenario], static_cast<unsigned long long>(iterations), seconds * 1000000000.0 / iterations);
//            }
//            std::printf("SINK,%llu\n", static_cast<unsigned long long>(checksum));
//            return 0;
//        }
//#if NEW_VARIANT_TEST
//        const size_t sizes[] = {0, 1, 7, 8, 15, 16, 17, 31, 64, 4096, 65536};
//        for (size_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index)
//        {
//            std::string expected(sizes[index], 'x');
//            for (size_t i = 0; i < expected.size(); ++i)
//            {
//                expected[i] = static_cast<char>(i * 37);
//            }
//            for (uint32_t kind = 0; kind < 2; ++kind)
//            {
//                const Variant::VariantType type = kind == 0 ? Variant::STRING : Variant::BLOB;
//                Variant first;
//                Check(first.assignBytes(expected.data(), expected.size(), type), "length ingestion");
//                Check(first.type() == type && first.size() == expected.size() && first.toBlob() == expected, "exact bytes");
//                Check(first.size() == 0 ? first.data() == nullptr : std::memcmp(first.data(), expected.data(), first.size()) == 0, "borrow view");
//                Variant copied(first);
//                Variant moved(std::move(first));
//                Check(first.type() == Variant::INIT && first.size() == 0 && moved == copied, "move source reset");
//                copied = copied;
//                moved = std::move(moved);
//                Check(moved.toBlob() == expected && copied == moved, "self assignment");
//                Variant assigned;
//                Check(&(assigned = moved) == &assigned && assigned == moved, "reference assignment");
//                assigned = std::move(moved);
//                Check(moved.type() == Variant::INIT && assigned.toBlob() == expected, "move assignment");
//                const Variant& readOnly = assigned;
//                Check(static_cast<std::string>(readOnly) == expected, "const text read");
//                if (expected.size() > 1)
//                {
//                    const std::string suffix = expected.substr(1);
//                    Check(assigned.assignBytes(assigned.data() + 1, assigned.size() - 1, type) && assigned.toBlob() == suffix, "self slice");
//                }
//                Check(!assigned.assignBytes(nullptr, 1) && assigned.type() == type, "invalid pointer rollback");
//                Variant empty;
//                assigned.swap(empty);
//                Check(assigned.type() == Variant::INIT, "swap empty");
//            }
//        }
//        for (int32_t type = -1; type <= 7; ++type)
//        {
//            for (size_t size = 0; size <= 33; ++size)
//            {
//                const bool valid = (type == 0 && size == 0) || (type == 1 && size == 4) ||
//                    ((type == 2 || type == 3) && size == 8) || type == 4 || type == 5;
//                char bytes[33] = {};
//                Variant value(int32_t(42));
//                Check(Variant::isValidBlob(size, type) == valid, "length validation");
//                Check(value.assignBytes(bytes, size, static_cast<Variant::VariantType>(type)) == valid, "ingestion validation");
//                if (!valid)
//                {
//                    Check(value.type() == Variant::INT32 && static_cast<int32_t>(value) == 42, "failure strong state");
//                }
//            }
//        }
//        uint32_t random = 0x12345678;
//        for (uint32_t iteration = 0; iteration < 20000; ++iteration)
//        {
//            random = random * 1664525U + 1013904223U;
//            int32_t number = 0;
//            std::memcpy(&number, &random, sizeof(number));
//            Variant value(number);
//            std::ostringstream reference;
//            reference.imbue(std::locale::classic());
//            reference << number;
//            Check(value.toString() == reference.str(), "integer decimal reference");
//            const uint64_t large = (static_cast<uint64_t>(random) << 32) | (random ^ 0xFEDCBA98U);
//            Variant wide(large);
//            std::ostringstream wideReference;
//            wideReference.imbue(std::locale::classic());
//            wideReference << large;
//            Check(wide.toString() == wideReference.str(), "uint64 decimal reference");
//        }
//        Check(Variant(int32_t(INT32_MIN)).toString() == "-2147483648" && Variant(uint64_t(UINT64_MAX)).toString() == "18446744073709551615", "decimal limits");
//#if defined(_MSC_VER) && defined(_DEBUG)
//        static uint32_t s_seen = 0;
//        static uint32_t s_failAt = 0;
//        // 按CRT约定返回1允许分配、0拒绝；只在测试范围记录普通堆块。
//        const auto Hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
//        {
//            return operation == _HOOK_ALLOC && block == _NORMAL_BLOCK && ++s_seen == s_failAt ? 0 : 1;
//        };
//        const char shortText[] = "1234567890123456";
//        s_seen = 0;
//        _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(Hook);
//        for (uint32_t i = 0; i < 1000; ++i)
//        {
//            Variant first(shortText);
//            Variant second(first);
//            second = first;
//            Variant third(std::move(second));
//        }
//        _CrtSetAllocHook(previous);
//        Check(s_seen == 0, "small bytes must not allocate");
//        const std::string large(4096, 'x');
//        Variant original(large);
//        s_seen = 0;
//        previous = _CrtSetAllocHook(Hook);
//        Variant moved(std::move(original));
//        original = std::move(moved);
//        _CrtSetAllocHook(previous);
//        Check(s_seen == 0, "heap move must not allocate");
//        _CrtMemState before;
//        _CrtMemState after;
//        _CrtMemCheckpoint(&before);
//        for (s_failAt = 1; s_failAt <= 8; ++s_failAt)
//        {
//            Variant value(int32_t(73));
//            s_seen = 0;
//            previous = _CrtSetAllocHook(Hook);
//            const bool accepted = value.assignBytes(large.data(), large.size());
//            _CrtSetAllocHook(previous);
//            Check(accepted ? value.toBlob() == large : static_cast<int32_t>(value) == 73, "allocation rollback");
//        }
//        s_failAt = 0;
//        _CrtMemCheckpoint(&after);
//        Check(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK], "allocation failure leak");
//#endif
//#endif
//        std::printf("Variant performance API checks=%u failures=%u sizeof=%llu\n", checks, failures, static_cast<unsigned long long>(sizeof(Variant)));
//        return failures == 0 ? 0 : 1;
//    }
//    catch (...)
//    {
//        std::printf("FAIL unexpected exception checks=%u\n", checks);
//        return 1;
//    }
//}
// VARIANT_PERF_UNIT_END