#include "VariantTest.h"
#include "Variant/VariantAPI.h"
#include "PlatformTest/PlatformTestAPI.h"
#include <cstdlib>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <locale>
#include <sstream>
#include <utility>
#include <vector>
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

/** 核对 Variant 的二进制输入、对象移动、十进制输出与内存故障；可单独运行十二项性能场景
@param [in] benchmark true 时只计时，false 时逐项验证公开接口
@param [in] selected 负值选择所有性能用例，0 到 11 选择对应编号；只在计时模式使用
@return 0 表示全部通过，非零表示输入或结果不符合预期
*/
int RunVariantScenarios(bool benchmark, int selected)
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
        if (benchmark)
        {
            const char* names[] = {"scalar_construct", "scalar_assign", "blob16_assign", "blob64_assign", "blob4096_assign",
                "blob16_copy", "blob64_copy", "integer_text", "blob4096_copy_read", "blob4096_move_chain", "raw64_ingest", "blob4096_borrow"};
            std::string payloads[] = {std::string(16, 'x'), std::string(64, 'y'), std::string(4096, 'z')};
            Variant source16(payloads[0]);
            Variant source64(payloads[1]);
            Variant source4096(payloads[2]);
            Variant target;
            for (uint32_t scenario = 0; scenario < 12; ++scenario)
            {
                if (selected >= 0 && scenario != static_cast<uint32_t>(selected))
                {
                    continue;
                }
                LibraryBenchmark::run(names[scenario], [&](uint32_t iterations) -> uint64_t
                {
                    uint64_t checksum = 0;
                    for (uint32_t i = 0; i < iterations; ++i)
                    {
                        switch (scenario)
                        {
                        case 0:
                            {
                                Variant value(static_cast<int32_t>(i));
                                checksum += static_cast<int32_t>(value);
                                break;
                            }
                        case 1:
                            target = static_cast<int32_t>(i);
                            checksum += static_cast<int32_t>(target);
                            break;
                        case 2:
                        case 3:
                        case 4:
                            target = payloads[scenario - 2];
                            checksum += target.type();
                            break;
                        case 5:
                            {
                                Variant value(source16);
                                checksum += value.type();
                                break;
                            }
                        case 6:
                            {
                                Variant value(source64);
                                checksum += value.type();
                                break;
                            }
                        case 7:
                            {
                                Variant value(static_cast<int32_t>(-1234567));
                                checksum += value.toString().size();
                                break;
                            }
                        case 8:
                            checksum += source4096.toBlob().size();
                            break;
                        case 9:
                            {
                                Variant first(payloads[2]);
                                Variant second(std::move(first));
                                Variant third(std::move(second));
                                checksum += third.type();
                                break;
                            }
                        case 10:
                            {
                                if (!target.assignBytes(payloads[1].data(), payloads[1].size()))
                                {
                                    throw std::runtime_error("Variant raw64_ingest failed");
                                }
                                checksum += target.type();
                                break;
                            }
                        case 11:
                            {
                                const char* bytes = source4096.data();
                                checksum += source4096.size() + static_cast<unsigned char>(bytes[0]);
                                break;
                            }
                        }
                    }
                    return checksum;
                });
            }
            return 0;
        }
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
                Check(first.size() == 0 ? first.data() == nullptr : std::memcmp(first.data(), expected.data(), first.size()) == 0, "borrow view");
                Variant copied(first);
                Variant moved(std::move(first));
                Check(first.type() == Variant::INIT && first.size() == 0 && moved == copied, "move source reset");
                copied = copied;
                moved = std::move(moved);
                Check(moved.toBlob() == expected && copied == moved, "self assignment");
                Variant assigned;
                Check(&(assigned = moved) == &assigned && assigned == moved, "reference assignment");
                assigned = std::move(moved);
                Check(moved.type() == Variant::INIT && assigned.toBlob() == expected, "move assignment");
                const Variant& readOnly = assigned;
                Check(static_cast<std::string>(readOnly) == expected, "const text read");
                if (expected.size() > 1)
                {
                    const std::string suffix = expected.substr(1);
                    Check(assigned.assignBytes(assigned.data() + 1, assigned.size() - 1, type) && assigned.toBlob() == suffix, "self slice");
                }
                Check(!assigned.assignBytes(nullptr, 1) && assigned.type() == type, "invalid pointer rollback");
                Variant empty;
                assigned.swap(empty);
                Check(assigned.type() == Variant::INIT, "swap empty");
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
                if (!valid)
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
        _CRT_ALLOC_HOOK previousFree = _CrtSetAllocHook(observe);
        const bool shortened = shrinking.assignBytes(shrinking.data() + 32, 64);
        shrinking.clear();
        _CrtSetAllocHook(previousFree);
        trackedBuffer = nullptr;
        Check(shortened && observedFree && entirelyErased, "shrunk allocation fully erased before release");
        const char shortText[] = "1234567890123456";
        s_seen = 0;
        _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(Hook);
        for (uint32_t i = 0; i < 1000; ++i)
        {
            Variant first(shortText);
            Variant second(first);
            second = first;
            Variant third(std::move(second));
        }
        _CrtSetAllocHook(previous);
        Check(s_seen == 0, "small bytes must not allocate");
        const std::string large(4096, 'x');
        Variant original(large);
        s_seen = 0;
        previous = _CrtSetAllocHook(Hook);
        Variant moved(std::move(original));
        original = std::move(moved);
        _CrtSetAllocHook(previous);
        Check(s_seen == 0, "heap move must not allocate");
        _CrtMemState before;
        _CrtMemState after;
        _CrtMemCheckpoint(&before);
        for (s_failAt = 1; s_failAt <= 8; ++s_failAt)
        {
            Variant value(int32_t(73));
            s_seen = 0;
            previous = _CrtSetAllocHook(Hook);
            const bool accepted = value.assignBytes(large.data(), large.size());
            _CrtSetAllocHook(previous);
            Check(accepted ? value.toBlob() == large : static_cast<int32_t>(value) == 73, "allocation rollback");
        }
        s_failAt = 0;
        _CrtMemCheckpoint(&after);
        Check(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK], "allocation failure leak");
#endif
        std::printf("Variant performance API checks=%u failures=%u sizeof=%llu\n", checks, failures, static_cast<unsigned long long>(sizeof(Variant)));
        return failures == 0 ? 0 : 1;
    }
    catch (...)
    {
        std::printf("FAIL unexpected exception checks=%u\n", checks);
        return 1;
    }
}