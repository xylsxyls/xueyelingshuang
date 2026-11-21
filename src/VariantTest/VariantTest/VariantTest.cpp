#include "PlatformTest/PlatformTestAPI.h"
#include "VariantTest.h"
#include <cstring>
#include "Variant/VariantAPI.h"
#include <cstdio>
#include <cstdlib>
#include <clocale>
#include <limits>
#if defined(_MSC_VER) && !defined(_DLL)
#include <windows.h>
#endif
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

/** 对本库的一项固定规模操作连续采样，结果用于同平台的改动前后比较。 */
static void RunBenchmark()
{
    Variant value;
    const std::string payload(1024, 'x');
    LibraryBenchmark::run("Variant", [&](uint32_t count) -> uint64_t
    {
        uint64_t checksum = 0;
        for (uint32_t index = 0; index < count; ++index)
        {
            value = payload;
            checksum += value.size();
        }
        return checksum;
    });
}

/** 核对数值位表示、字节所有权与分配异常下的恢复。 */
static int TestOwnership()
{
    uint32_t checks = 0;
    // condition为期望结果，message为静态失败说明；异常仅由本main捕获。
    const auto Check = [&checks](bool condition, const char* message)
    {
        ++checks;
        if (!condition)
        {
            std::printf("FAIL %s\n", message);
            throw 1;
        }
    };
    try
    {
        const bool standard = static_cast<double>(Variant("  -.25e1suffix", Variant::DOUBLE)) == -2.5;
        Check(standard, "canonical fraction with exponent");
#ifdef _WIN32
        bool usesTestLocale = true;
#if defined(_MSC_VER) && !defined(_DLL)
        // /MT 的区域设置属于各自模块，静态链接时 Variant 和测试程序共用一份 CRT
        HMODULE owner = nullptr;
        const auto validateSize = static_cast<bool (*)(size_t, int32_t)>(&Variant::isValidBlob);
        const BOOL located = GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(validateSize), &owner);
        Check(located != 0, "locate the module containing Variant");
        usesTestLocale = owner == GetModuleHandleW(nullptr);
#endif
        // 单线程用例先保存区域设置，恢复之后再执行可能抛出的断言
        const std::string savedLocale = std::setlocale(LC_NUMERIC, nullptr);
        const bool commaLocale = std::setlocale(LC_NUMERIC, "German_Germany.1252") != nullptr &&
            std::strcmp(std::localeconv()->decimal_point, ",") == 0;
        const Variant original(-1234.25);
        const bool roundTrip = static_cast<double>(Variant(original.toString(), Variant::DOUBLE)) == -1234.25;
        const bool signedFraction = static_cast<double>(Variant("  -.25e1suffix", Variant::DOUBLE)) == -2.5;
        // 独立 CRT 保持初始 C 区域，逗号会终止 strtod 的数字前缀
        const double expectedLocal = usesTestLocale ? 1.25 : 1.0;
        const bool local = static_cast<double>(Variant("1,25", Variant::DOUBLE)) == expectedLocal;
        const bool restored = std::setlocale(LC_NUMERIC, savedLocale.c_str()) != nullptr;
        Check(commaLocale && roundTrip && signedFraction && local && restored, "comma locale canonical and local fractions");
#endif
        const std::string first(64, 'a');
        Check(static_cast<int32_t>(Variant("2147483648", Variant::INT32)) == INT32_MAX, "positive text overflow saturates");
        Check(static_cast<int32_t>(Variant("-2147483649", Variant::INT32)) == INT32_MIN, "negative text overflow saturates");
        Check(static_cast<int32_t>(Variant(std::string(80, '9'), Variant::INT32)) == INT32_MAX, "arbitrarily long text saturates");
        Check(static_cast<int32_t>(Variant("  -12tail", Variant::INT32)) == -12, "decimal prefix is preserved");
        const std::string second(64, 'b');
        Variant reused(first + second);
        Check(reused.assignBytes(reused.data() + 32, 80) && reused.toBlob() == (first + second).substr(32, 80),
            "overlapping long blob assignment preserves original bytes");
        const int32_t signedValues[] = {0, -1, INT32_MIN, INT32_MAX, 1234567};
        const uint64_t unsignedValues[] = {0, 1, UINT64_MAX, UINT64_C(0xfedcba9876543210)};
        for (uint32_t pass = 0; pass < 1000; ++pass)
        {
            for (size_t index = 0; index < sizeof(signedValues) / sizeof(signedValues[0]); ++index)
            {
                Variant value(signedValues[index]);
                Variant copied(value);
                value.clear();
                Check(value.type() == Variant::INIT && copied.type() == Variant::INT32 && static_cast<int32_t>(copied) == signedValues[index],
                    "signed scalar copy or cleanup");
                copied = copied;
                Check(static_cast<int32_t>(copied) == signedValues[index] && static_cast<uint64_t>(copied) == 0, "self assignment or wrong-type read");
                const std::string raw(reinterpret_cast<const char*>(&signedValues[index]), sizeof(int32_t));
                Check(copied.toBlob() == raw, "signed native bytes changed");
                Variant decoded(raw, Variant::INT32, true);
                Check(static_cast<int32_t>(decoded) == signedValues[index], "signed blob round trip");
            }
            for (size_t index = 0; index < sizeof(unsignedValues) / sizeof(unsignedValues[0]); ++index)
            {
                Variant value(unsignedValues[index]);
                Variant assigned;
                Variant returned = assigned = value;
                value.clear();
                Check(static_cast<uint64_t>(assigned) == unsignedValues[index] && returned == assigned,
                    "unsigned assignment snapshot");
                assigned = static_cast<int32_t>(-7);
                Check(static_cast<uint64_t>(returned) == unsignedValues[index] && static_cast<int32_t>(assigned) == -7,
                    "assignment return value must remain independent");
            }
        }
        const uint64_t floatBits[] = {0, UINT64_C(0x8000000000000000), UINT64_C(0x3ff4000000000000),
            UINT64_C(0x7ff0000000000000), UINT64_C(0x7ff8000000001234)};
        for (size_t index = 0; index < sizeof(floatBits) / sizeof(floatBits[0]); ++index)
        {
            double number = 0;
            std::memcpy(&number, &floatBits[index], sizeof(number));
            Variant value(number);
            Variant copy(value);
            double actual = static_cast<double>(copy);
            uint64_t bits = 0;
            std::memcpy(&bits, &actual, sizeof(bits));
            Check(bits == floatBits[index], "double bit representation changed");
        }
        const std::string bytes("A\0B\xff", 4);
        Variant blob(bytes);
        // 文本入口按首个零字节截断，显式长度入口和原始反序列化按完整长度保存。
        Variant ordinaryText(bytes.c_str());
        Variant explicitText(bytes, Variant::STRING, false);
        Variant explicitBlob(bytes, Variant::BLOB, false);
        Variant assignedText;
        assignedText = bytes.c_str();
        Variant assignedBlob;
        assignedBlob = bytes;
        const Variant expectedText("A");
        Check(ordinaryText.type() == Variant::STRING && ordinaryText == expectedText &&
            explicitText.type() == Variant::STRING && explicitText == expectedText &&
            assignedText.type() == Variant::STRING && assignedText == expectedText, "text stops at first NUL");
        Check(explicitBlob.type() == Variant::BLOB && explicitBlob == blob &&
            assignedBlob.type() == Variant::BLOB && assignedBlob == blob, "blob preserves embedded NUL");
        Variant zeroFirst(std::string("\0A", 2), Variant::STRING, false);
        Variant zeroBlob(std::string("\0A", 2));
        Check(zeroFirst.type() == Variant::STRING && zeroFirst == Variant("") &&
            zeroBlob.type() == Variant::BLOB && zeroBlob != zeroFirst, "leading NUL text and blob distinction");
        Variant text(bytes, Variant::STRING, true);
        Check(blob == text && blob.type() == Variant::BLOB && text.type() == Variant::STRING, "historical byte-only comparison changed");
        Variant textCopy(text);
        Variant textAssigned;
        textAssigned = text;
        Check(textCopy.type() == Variant::STRING && textCopy == blob &&
            textAssigned.type() == Variant::STRING && textAssigned == blob &&
            Variant::isValidBlob(bytes, Variant::STRING), "serialized STRING preserves bytes and type");
        Variant copy(blob);
        blob.clear();
        Check(copy == text && copy != blob, "byte ownership not independent");
        Variant empty;
        Variant emptyText(static_cast<const char*>(nullptr));
        Check(empty == emptyText && emptyText.type() == Variant::STRING, "empty value comparison changed");
        for (int32_t type = -1; type <= 7; ++type)
        {
            for (uint32_t length = 0; length <= 9; ++length)
            {
                const std::string data(length, 'x');
                const bool valid = (type == 0 && length == 0) || (type == 1 && length == 4) ||
                    ((type == 2 || type == 3) && length == 8) || type == 4 || type == 5;
                Check(Variant::isValidBlob(data, type) == valid, "variant width validation");
                Variant value(data, static_cast<Variant::VariantType>(type), true);
                Check(value.type() == (valid ? static_cast<Variant::VariantType>(type) : Variant::INIT), "invalid variant constructed");
            }
        }
        Check(copy.toBlob() == bytes && static_cast<std::string>(text) == bytes && text.toString() == bytes, "binary/text output truncation");
        Variant negative(static_cast<int32_t>(-123));
        Variant maximum(static_cast<uint64_t>(UINT64_MAX));
        Variant fraction(1.25);
        Check(negative.toString() == "-123" && maximum.toString() == "18446744073709551615" && fraction.toString() == "1.25",
            "numeric text compatibility");
        Variant emptyNumber(std::string(), Variant::UINT64, false);
        Check(emptyNumber.type() == Variant::UINT64 && emptyNumber.toBlob().empty() && static_cast<uint64_t>(emptyNumber) == 0,
            "legacy empty numeric text changed");
#if defined(_MSC_VER) && defined(_DEBUG)
        static uint32_t s_seen = 0;
        static uint32_t s_failAt = 0;
        // 标准CRT钩子参数；只拒绝第s_failAt项普通分配，0只计数。
        const auto Hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
        {
            if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK)
            {
                return ++s_seen == s_failAt ? 0 : 1;
            }
            return 1;
        };
        s_seen = 0;
        _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(Hook);
        for (uint32_t index = 0; index < 10000; ++index)
        {
            Variant first(static_cast<uint64_t>(index));
            Variant second(first);
            second = static_cast<int32_t>(index);
        }
        _CrtSetAllocHook(previous);
        Check(s_seen == 0, "scalar construction/copy/assignment allocated");
        const std::string payload(100, 'x');
        _CrtMemState before;
        _CrtMemState after;
        _CrtMemCheckpoint(&before);
        uint32_t rejected = 0;
        for (s_failAt = 1; s_failAt <= 16; ++s_failAt)
        {
            Variant original(static_cast<int32_t>(17));
            s_seen = 0;
            previous = _CrtSetAllocHook(Hook);
            try
            {
                Variant value(payload);
                Variant copied(value);
                original = copied;
            }
            catch (...)
            {
                ++rejected;
            }
            _CrtSetAllocHook(previous);
            Check(original.type() == Variant::INT32 || original.type() == Variant::BLOB, "failed assignment corrupted type");
            original = static_cast<uint64_t>(19);
            Check(static_cast<uint64_t>(original) == 19, "assignment did not recover after failure");
        }
        _CrtMemCheckpoint(&after);
        Check(rejected != 0 && before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] &&
            before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK], "variant construction/assignment leaked on failure");
#endif
        std::printf("Variant ownership checks=%u PASS\n", checks);
        return 0;
    }
    catch (...)
    {
        std::printf("Variant ownership checks=%u FAIL\n", checks);
        return 1;
    }
}

/** 按稳定功能用例号或显式性能参数运行。 */
int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        RunBenchmark();
        return RunVariantScenarios(true, -1);
    }
    if (argc >= 2 && std::strcmp(argv[1], "--benchmark-operations") == 0)
    {
        if (argc == 2)
        {
            return RunVariantScenarios(true, -1);
        }
        if (argc != 3)
        {
            return 2;
        }
        char* end = nullptr;
        const long selected = std::strtol(argv[2], &end, 10);
        if (end == argv[2] || *end != '\0' || std::strspn(argv[2], "0123456789") != std::strlen(argv[2]) || selected < 0 || selected > 11)
        {
            return 2;
        }
        return RunVariantScenarios(true, static_cast<int>(selected));
    }
    const LibraryTestCase cases[] = {
        {1, false, TestOwnership},
        {2, true, []() { return RunVariantScenarios(false, -1); }}
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}