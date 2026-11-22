#include "VariantTest.h"
#include "Variant/VariantAPI.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <locale>
#include <sstream>
#include <utility>
#include <stdexcept>

/** 用同一操作完成预热、校准和九轮计时，每批比较一次校验和
@param [in] timed true 使用公共计时器，false 仅运行固定批量核对结果
@param [in] name 输出场景名
@param [in] expected 每项固定贡献；-1 表示贡献等于从零开始的迭代下标
@param [in] operation 接收迭代下标并返回本次操作的可观察结果
*/
template<class Operation>
static void RunOperation(bool timed, const char* name, int32_t expected, const Operation& operation)
{
    const auto batch = [&](uint32_t count) -> uint64_t
    {
        uint64_t checksum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            checksum += operation(i);
        }
        const uint64_t n = count;
        const uint64_t required = expected < 0 ? n * (n - 1) / 2 : n * expected;
        if (checksum != required)
        {
            throw std::runtime_error(name);
        }
        return checksum;
    };
    if (timed)
    {
        LibraryBenchmark::run(name, batch);
        return;
    }
    const uint32_t counts[] = {1, 2, 3, 127, 128};
    for (size_t i = 0; i < sizeof(counts) / sizeof(counts[0]); ++i)
    {
        batch(counts[i]);
    }
}

/** 观察字节值的类型和首字节，让正文参与每批校验和
@param [in] value 本次操作产生的非空值
@return 类型号加无符号首字节；缺少正文时只返回类型号，批校验会失败
*/
static uint32_t SampleBlob(const Variant& value)
{
    const char* bytes = value.data();
    return value.type() + (bytes == nullptr ? 0 : static_cast<unsigned char>(bytes[0]));
}

/** 从指定的起止编号执行工作负载
@param [in] first 首个操作编号
@param [in] last 最后一个操作编号，12 为独立的 1024 字节复用场景
@param [in] timed 是否调用公共基准计时器
@return 完成返回 0，校验或分配失败返回 1
*/
static int RunOperations(uint32_t first, uint32_t last, bool timed)
{
    try
    {
        const char* names[] = {"scalar_construct", "scalar_assign", "blob16_assign", "blob64_assign", "blob4096_assign",
            "blob16_copy", "blob64_copy", "integer_text", "blob4096_copy_read", "blob4096_move_chain", "raw64_ingest", "blob4096_borrow", "Variant"};
        std::string payloads[] = {std::string(16, 'x'), std::string(64, 'y'), std::string(4096, 'z')};
        Variant source16(payloads[0]);
        Variant source64(payloads[1]);
        Variant source4096(payloads[2]);
        for (uint32_t scenario = first; scenario <= last; ++scenario)
        {
            Variant target;
            // 选择场景和准备输入都在计时循环之外。
            switch (scenario)
            {
            case 0:
                RunOperation(timed, names[scenario], -1, [](uint32_t i)
                {
                    Variant value(static_cast<int32_t>(i));
                    return static_cast<int32_t>(value);
                });
                break;
            case 1:
                RunOperation(timed, names[scenario], -1, [&](uint32_t i)
                {
                    target = static_cast<int32_t>(i);
                    return static_cast<int32_t>(target);
                });
                break;
            case 2:
            case 3:
            case 4:
                {
                    const std::string& payload = payloads[scenario - 2];
                    RunOperation(timed, names[scenario], Variant::BLOB + payload[0], [&](uint32_t)
                    {
                        target = payload;
                        return SampleBlob(target);
                    });
                    break;
                }
            case 5:
            case 6:
                {
                    const Variant& source = scenario == 5 ? source16 : source64;
                    RunOperation(timed, names[scenario], Variant::BLOB + (scenario == 5 ? 'x' : 'y'), [&](uint32_t)
                    {
                        Variant copy(source);
                        return SampleBlob(copy);
                    });
                    break;
                }
            case 7:
                RunOperation(timed, names[scenario], 8, [](uint32_t)
                {
                    Variant value(static_cast<int32_t>(-1234567));
                    return value.toString().size();
                });
                break;
            case 8:
                RunOperation(timed, names[scenario], 4096 + 'z', [&](uint32_t)
                {
                    const std::string output = source4096.toBlob();
                    return output.size() + (output.empty() ? 0 : static_cast<unsigned char>(output[0]));
                });
                break;
            case 9:
                RunOperation(timed, names[scenario], Variant::BLOB + 'z', [&](uint32_t)
                {
                    Variant first(payloads[2]);
                    Variant second(std::move(first));
                    Variant third(std::move(second));
                    return SampleBlob(third);
                });
                break;
            case 10:
                RunOperation(timed, names[scenario], Variant::BLOB + 'y', [&](uint32_t)
                {
                    if (!target.assignBytes(payloads[1].data(), payloads[1].size()))
                    {
                        throw std::runtime_error("raw64_ingest");
                    }
                    return SampleBlob(target);
                });
                break;
            case 11:
                RunOperation(timed, names[scenario], 4096 + 'z', [&](uint32_t)
                {
                    const char* bytes = source4096.data();
                    return source4096.size() + (bytes == nullptr ? 0 : static_cast<unsigned char>(bytes[0]));
                });
                break;
            case 12:
                {
                    const std::string payload(1024, 'x');
                    RunOperation(timed, names[scenario], 1024 + Variant::BLOB + 'x', [&](uint32_t)
                    {
                        target = payload;
                        return target.size() + SampleBlob(target);
                    });
                    break;
                }
            }
        }
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr, "Variant benchmark failed: %s\n", error.what());
        return 1;
    }
}

int RunVariantBenchmarks(int selected)
{
    if (selected < -1 || selected > 11)
    {
        return 2;
    }
    return selected == -1 ? RunOperations(0, 11, true) : RunOperations(selected, selected, true);
}

int RunAllVariantBenchmarks()
{
    return RunOperations(12, 12, true) == 0 ? RunVariantBenchmarks(-1) : 1;
}

int CheckVariantBenchmarkBatches()
{
    if (RunVariantBenchmarks(-2) != 2 || RunVariantBenchmarks(12) != 2)
    {
        return 1;
    }
    return RunOperations(0, 12, false);
}

int RunVariantScenarios()
{
    uint32_t checks = 0;
    uint32_t failures = 0;
    // condition为独立预期，reason为静态说明；仅记录，不终止产品进程。
    const auto Check = [&checks, &failures](bool condition, const char* reason)
    {
        ++checks;
        if (!condition)
        {
            ++failures;
            std::printf("FAIL %s\n", reason);
        }
    };
    try
    {
        const size_t sizes[] = {0, 1, 7, 8, 15, 16, 17, 31, 64, 4096, 65536};
        for (size_t index = 0; index < sizeof(sizes) / sizeof(sizes[0]); ++index)
        {
            std::string expected(sizes[index], 'x');
            for (size_t i = 0; i < expected.size(); ++i)
            {
                expected[i] = static_cast<char>(i * 37);
            }
            for (uint32_t kind = 0; kind < 2; ++kind)
            {
                const Variant::VariantType type = kind == 0 ? Variant::STRING : Variant::BLOB;
                Variant first;
                Check(first.assignBytes(expected.data(), expected.size(), type), "length ingestion");
                Check(first.type() == type && first.size() == expected.size() && first.toBlob() == expected, "exact bytes");
                Check(expected.empty() ? first.data() == nullptr : first.size() == expected.size() && first.data() != nullptr &&
                    std::memcmp(first.data(), expected.data(), expected.size()) == 0, "borrow view");
                Variant copied(first);
                Variant moved(std::move(first));
                Check(first.type() == Variant::INIT && first.data() == nullptr && first.size() == 0 &&
                    moved.type() == type && copied.type() == type && moved == copied, "move source reset");
                copied = copied;
                moved = std::move(moved);
                Check(moved.type() == type && moved.toBlob() == expected && copied.type() == type && copied == moved, "self assignment");
                Variant assigned;
                Check(&(assigned = moved) == &assigned && assigned.type() == type && assigned == moved, "reference assignment");
                assigned = std::move(moved);
                Check(moved.type() == Variant::INIT && moved.data() == nullptr && moved.size() == 0 &&
                    assigned.type() == type && assigned.toBlob() == expected, "move assignment");
                const Variant& readOnly = assigned;
                Check(static_cast<std::string>(readOnly) == expected, "const text read");
                if (expected.size() > 1 && assigned.data() != nullptr && assigned.size() == expected.size())
                {
                    const std::string suffix = expected.substr(1);
                    Check(assigned.assignBytes(assigned.data() + 1, assigned.size() - 1, type) && assigned.toBlob() == suffix, "self slice");
                }
                const std::string retained = expected.size() > 1 ? expected.substr(1) : expected;
                Check(!assigned.assignBytes(nullptr, 1) && assigned.type() == type && assigned.toBlob() == retained, "invalid pointer rollback");
                Variant empty;
                assigned.swap(empty);
                Check(assigned.type() == Variant::INIT && assigned.data() == nullptr && empty.type() == type && empty.toBlob() == retained, "swap empty");
            }
        }
        for (int32_t type = -1; type <= 7; ++type)
        {
            for (size_t size = 0; size <= 33; ++size)
            {
                const bool valid = (type == 0 && size == 0) || (type == 1 && size == 4) ||
                    ((type == 2 || type == 3) && size == 8) || type == 4 || type == 5;
                char bytes[33] = {};
                Variant value(int32_t(42));
                Check(Variant::isValidBlob(size, type) == valid, "length validation");
                Check(value.assignBytes(bytes, size, static_cast<Variant::VariantType>(type)) == valid, "ingestion validation");
                if (valid)
                {
                    Check(value.type() == static_cast<Variant::VariantType>(type) && value.size() == size &&
                        value.toBlob() == std::string(bytes, size), "valid raw input commits type, width and bytes");
                }
                else
                {
                    Check(value.type() == Variant::INT32 && static_cast<int32_t>(value) == 42, "failure strong state");
                }
            }
        }
        uint32_t random = 0x12345678;
        for (uint32_t iteration = 0; iteration < 20000; ++iteration)
        {
            random = random * 1664525U + 1013904223U;
            int32_t number = 0;
            std::memcpy(&number, &random, sizeof(number));
            Variant value(number);
            std::ostringstream reference;
            reference.imbue(std::locale::classic());
            reference << number;
            Check(value.toString() == reference.str(), "integer decimal reference");
            const uint64_t large = (static_cast<uint64_t>(random) << 32) | (random ^ 0xFEDCBA98U);
            Variant wide(large);
            std::ostringstream wideReference;
            wideReference.imbue(std::locale::classic());
            wideReference << large;
            Check(wide.toString() == wideReference.str(), "uint64 decimal reference");
        }
        Check(Variant(int32_t(INT32_MIN)).toString() == "-2147483648" && Variant(uint64_t(UINT64_MAX)).toString() == "18446744073709551615", "decimal limits");
#if defined(_MSC_VER) && defined(_DEBUG)
        static uint32_t s_seen = 0;
        static uint32_t s_failAt = 0;
        // 按CRT约定返回1允许分配、0拒绝；只在测试范围记录普通堆块。
        const auto Hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
        {
            return operation == _HOOK_ALLOC && block == _NORMAL_BLOCK && ++s_seen == s_failAt ? 0 : 1;
        };
        // 只在 CRT 释放原分配块之前观察，绝不访问已经释放的内存。
        Variant shrinking(std::string(128, 'x'));
        static const char* trackedBuffer = nullptr;
        static bool observedFree = false;
        static bool entirelyErased = false;
        trackedBuffer = shrinking.data();
        observedFree = false;
        entirelyErased = false;
        const auto observe = [](int operation, void* block, size_t, int, long, const unsigned char*, int) -> int
        {
            if (operation == _HOOK_FREE && block == trackedBuffer)
            {
                observedFree = true;
                entirelyErased = std::count(trackedBuffer, trackedBuffer + 128, char(0)) == 128;
            }
            return 1;
        };
        bool shortened = false;
        {
            const VariantTestAllocHook guard(observe);
            shortened = shrinking.assignBytes(shrinking.data() + 32, 64);
            shrinking.clear();
        }
        trackedBuffer = nullptr;
        Check(shortened && observedFree && entirelyErased, "shrunk allocation fully erased before release");
        const char shortText[] = "1234567890123456";
        s_seen = 0;
        {
            const VariantTestAllocHook guard(Hook);
            for (uint32_t i = 0; i < 1000; ++i)
            {
                Variant first(shortText);
                Variant second(first);
                second = first;
                Variant third(std::move(second));
            }
        }
        Check(s_seen == 0, "small bytes must not allocate");
        const std::string large(4096, 'x');
        Variant original(large);
        s_seen = 0;
        {
            const VariantTestAllocHook guard(Hook);
            Variant moved(std::move(original));
            original = std::move(moved);
        }
        Check(s_seen == 0, "heap move must not allocate");
        _CrtMemState before;
        _CrtMemState after;
        _CrtMemCheckpoint(&before);
        uint32_t rejected = 0;
        for (s_failAt = 1; s_failAt <= 8; ++s_failAt)
        {
            Variant value(int32_t(73));
            s_seen = 0;
            bool accepted = false;
            {
                const VariantTestAllocHook guard(Hook);
                accepted = value.assignBytes(large.data(), large.size());
            }
            rejected += accepted ? 0 : 1;
            Check(accepted ? value.type() == Variant::BLOB && value.toBlob() == large :
                value.type() == Variant::INT32 && static_cast<int32_t>(value) == 73, "allocation rollback");
        }
        Check(rejected != 0, "fault injection reached assignBytes");
        s_failAt = 0;
        _CrtMemCheckpoint(&after);
        Check(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK], "allocation failure leak");
#endif
        std::printf("Variant scenario checks=%u failures=%u sizeof=%llu\n", checks, failures, static_cast<unsigned long long>(sizeof(Variant)));
        return failures == 0 ? 0 : 1;
    }
    catch (...)
    {
        std::printf("FAIL unexpected exception checks=%u\n", checks);
        return 1;
    }
}