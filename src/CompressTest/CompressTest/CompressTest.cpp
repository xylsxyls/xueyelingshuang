#include "CompressTest.h"
#include "PlatformTest/PlatformTestAPI.h"
#include "Compress/CompressAPI.h"
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif
#include <cstdio>
#include <cstring>
#include <vector>
#include <stdexcept>

// 失败时停止本例，保留首个错误的位置与名称。
#define COMPRESS_TEST_CHECK(value, message) do { \
    ++checks; \
    if (!(value)) \
    { \
        std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, message); \
        return 1; \
    } \
} while (false)

/** 独立检查全部压缩等级和输入模式
@param [in,out] checks 累加已验证的断言数
@return 通过返回0，失败返回1
*/
static int CheckRawLevels(int& checks)
{
    const uint32_t lengths[] = {0, 1, 4096, 131072};
    uint32_t random = 123456789;
    for (int pattern = 0; pattern < 2; ++pattern)
    {
        for (size_t index = 0; index < sizeof(lengths) / sizeof(lengths[0]); ++index)
        {
            const uint32_t length = lengths[index];
            std::vector<uint8_t> input(length + 1), plain(length + 1);
            for (uint32_t i = 0; i < length; ++i)
            {
                random ^= random << 13;
                random ^= random >> 17;
                random ^= random << 5;
                input[i] = pattern == 0 ? 'A' : static_cast<uint8_t>(random);
            }
            const uint32_t capacity = Compress::rawCompressBound(length);
            COMPRESS_TEST_CHECK(capacity >= length && capacity != 0, "raw bound");
            std::vector<uint8_t> packed(static_cast<size_t>(capacity) + 1);
            for (int level = 0; level <= 9; ++level)
            {
                uint32_t size = capacity;
                std::memset(packed.data(), 0, capacity);
                packed.back() = 0xa5;
                COMPRESS_TEST_CHECK(Compress::rawCompress(&packed[0], size, &input[0], length, level) == COMPRESS_OK, "C++ compression");
                COMPRESS_TEST_CHECK(size > 4 && size <= capacity && packed[0] == 0x28 &&
                    packed[1] == 0xb5 && packed[2] == 0x2f && packed[3] == 0xfd &&
                    (packed[4] & 4) != 0 && packed.back() == 0xa5, "checksummed standard frame within capacity");
                // 逐级重置，任何一个等级漏写明文都会与独立输入不同
                for (uint32_t i = 0; i < length; ++i)
                {
                    plain[i] = static_cast<uint8_t>(~input[i]);
                }
                plain.back() = 0x5a;
                uint32_t actual = length, consumed = size;
                COMPRESS_TEST_CHECK(Compress::rawUncompress(&plain[0], actual, &packed[0], consumed) == COMPRESS_OK, "C++ decompression");
                COMPRESS_TEST_CHECK(actual == length && consumed == size && plain.back() == 0x5a &&
                    std::memcmp(&plain[0], &input[0], length) == 0, "round-trip bytes, lengths and output guard");
            }
        }
    }
    return 0;
}

/** 验证原始帧、错误返回和首帧消费长度
@param [in,out] checks 累加已验证的断言数
@return 通过返回0，失败返回1
*/
static int CheckRawFrames(int& checks)
{
    uint8_t input[128], packed[256], output[128];
    std::memset(input, 'A', sizeof(input));
    uint32_t size = sizeof(packed);
    COMPRESS_TEST_CHECK(Compress::rawCompressBound(UINT32_MAX) == 0, "overflow bound");
    COMPRESS_TEST_CHECK(Compress::rawCompress(packed, size, input, sizeof(input), 10) == COMPRESS_INVALID_ARGUMENT && size == 0, "invalid level");
    size = sizeof(packed);
    COMPRESS_TEST_CHECK(Compress::rawCompress(packed, size, input, sizeof(input), -1) == COMPRESS_INVALID_ARGUMENT &&
        size == 0, "negative raw compression level");
    size = sizeof(packed);
    COMPRESS_TEST_CHECK(Compress::rawCompress(packed, size, nullptr, 1) == COMPRESS_INVALID_ARGUMENT && size == 0, "null source");
    size = sizeof(packed);
    COMPRESS_TEST_CHECK(Compress::rawCompress(nullptr, size, input, 1) == COMPRESS_INVALID_ARGUMENT && size == 0, "null output");
    size = 1;
    COMPRESS_TEST_CHECK(Compress::rawCompress(packed, size, input, sizeof(input)) == COMPRESS_BUFFER_TOO_SMALL && size == 0, "small compression buffer");
    size = sizeof(packed);
    COMPRESS_TEST_CHECK(Compress::rawCompress(packed, size, input, sizeof(input)) == COMPRESS_OK, "valid stream for negative cases");
    COMPRESS_TEST_CHECK(size > 1 && size < sizeof(packed), "room for a trailing byte");
    const uint32_t packedSize = size;
    uint32_t actual = sizeof(output), consumed = packedSize + 1;
    packed[packedSize] = 42;
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, packed, consumed) == COMPRESS_OK &&
        actual == sizeof(input) && consumed == packedSize && std::memcmp(output, input, actual) == 0, "first frame with trailing bytes");
    actual = 1;
    consumed = packedSize;
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, packed, consumed) == COMPRESS_BUFFER_TOO_SMALL && actual == 0 && consumed == 0, "small decompression buffer");
    actual = sizeof(output);
    consumed = packedSize;
    packed[packedSize - 1] ^= 1;
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, packed, consumed) == COMPRESS_DATA_ERROR &&
        actual == 0 && consumed == 0, "raw checksum mismatch");
    packed[packedSize - 1] ^= 1;
    actual = sizeof(output);
    consumed = packedSize - 1;
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, packed, consumed) == COMPRESS_DATA_ERROR && actual == 0 && consumed == 0, "truncated stream");
    packed[0] ^= 255;
    actual = sizeof(output);
    consumed = packedSize;
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, packed, consumed) == COMPRESS_DATA_ERROR && actual == 0 && consumed == 0, "corrupt stream");
    // 以其他压缩格式和独立构造的标准帧验证解码器的输入边界
    const uint8_t known[] = {0x78, 0x9c, 0xcb, 0x48, 0xcd, 0xc9, 0xc9, 0x07, 0x00, 0x06, 0x2c, 0x02, 0x15};
    actual = sizeof(output);
    consumed = sizeof(known);
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, known, consumed) == COMPRESS_DATA_ERROR && actual == 0 && consumed == 0, "non-Zstandard input rejected");
    const uint8_t frame[] = {0x28, 0xb5, 0x2f, 0xfd, 0x20, 0x05, 0x29, 0x00, 0x00, 'h', 'e', 'l', 'l', 'o'};
    actual = sizeof(output);
    consumed = sizeof(frame);
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, frame, consumed) == COMPRESS_OK &&
        actual == 5 && consumed == sizeof(frame) && std::memcmp(output, "hello", 5) == 0, "independent Zstandard frame");
    std::vector<uint8_t> joined(frame, frame + sizeof(frame));
    joined.insert(joined.end(), frame, frame + sizeof(frame));
    actual = sizeof(output);
    consumed = static_cast<uint32_t>(joined.size());
    std::memset(output, 0, sizeof(output));
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, joined.data(), consumed) == COMPRESS_OK &&
        actual == 5 && consumed == sizeof(frame) && std::memcmp(output, "hello", 5) == 0, "only first complete frame consumed");
    uint32_t sharedLength = sizeof(frame);
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, sharedLength, frame, sharedLength) == COMPRESS_INVALID_ARGUMENT &&
        sharedLength == 0, "distinct output and consumption lengths required");
    actual = sizeof(output);
    consumed = sizeof(frame);
    COMPRESS_TEST_CHECK(Compress::rawUncompress(nullptr, actual, frame, consumed) == COMPRESS_INVALID_ARGUMENT &&
        actual == 0 && consumed == 0, "null decode destination");
    actual = sizeof(output);
    consumed = sizeof(frame);
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, nullptr, consumed) == COMPRESS_INVALID_ARGUMENT &&
        actual == 0 && consumed == 0, "null decode source");
    const uint8_t skip[] = {0x50, 0x2a, 0x4d, 0x18, 0, 0, 0, 0};
    actual = sizeof(output);
    consumed = sizeof(skip);
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, skip, consumed) == COMPRESS_DATA_ERROR &&
        actual == 0 && consumed == 0, "skippable frame rejected");
    return 0;
}

/** 独立构造一个省略内容长度的标准Zstandard RLE块
@return 带本库四字节头的测试报文，解码值是65537个Z
*/
static std::string MakeUnknownSizeFrame()
{
    const uint8_t stream[] = {0x28, 0xb5, 0x2f, 0xfd, 0, 0x40, 0x0b, 0, 0x08, 'Z'};
    const uint32_t streamLength = 65537;
    std::string framed(sizeof(streamLength), '\0');
    std::memcpy(&framed[0], &streamLength, sizeof(streamLength));
    framed.append(reinterpret_cast<const char*>(stream), sizeof(stream));
    return framed;
}

/** 验证四字节头、缓冲、字符串和原地调用
@param [in,out] checks 累加已验证的断言数
@return 通过返回0，失败返回1
*/
static int CheckWrapped(int& checks)
{
    const uint32_t streamLength = 65537;
    const std::string framed = MakeUnknownSizeFrame();
    std::vector<char> raw(streamLength + 1, '!');
    uint32_t returned = streamLength;
    Compress::uncompress(raw.data(), returned, framed.data(), static_cast<uint32_t>(framed.size()));
    const std::string expected(streamLength, 'Z');
    COMPRESS_TEST_CHECK(returned == streamLength && std::memcmp(raw.data(), expected.data(), streamLength) == 0 &&
        raw.back() == '!', "external frame with no stored size decodes into caller buffer");
    COMPRESS_TEST_CHECK(Compress::uncompress(framed) == expected, "string accepts external frame without stored size");
    std::string changed = framed;
    Compress::uncompress(changed, changed, streamLength);
    COMPRESS_TEST_CHECK(changed == expected, "in-place streaming frame at exact application budget");
    Compress::uncompress(changed, framed, streamLength - 1);
    COMPRESS_TEST_CHECK(changed.empty(), "streaming frame obeys application budget");
    const uint32_t forgedLength = UINT32_MAX;
    changed = framed;
    std::memcpy(&changed[0], &forgedLength, sizeof(forgedLength));
    COMPRESS_TEST_CHECK(Compress::uncompress(changed).empty(), "unknown frame size does not trust forged outer length");
    const uint32_t shortDeclaration = streamLength - 1;
    std::memcpy(&changed[0], &shortDeclaration, sizeof(shortDeclaration));
    std::string shortResult("previous");
    Compress::uncompress(shortResult, changed, shortDeclaration);
    COMPRESS_TEST_CHECK(shortResult.empty(), "actual streamed output exceeds the admitted size");
    // 单段帧头谎报4 GiB，但正文只有一个字节，不能按声明分配大缓冲区。
    const uint8_t falseSize[] = {0x28, 0xb5, 0x2f, 0xfd, 0xa0, 0xff, 0xff, 0xff, 0xff, 9, 0, 0, 'Z'};
    changed.assign(sizeof(forgedLength), '\0');
    std::memcpy(&changed[0], &forgedLength, sizeof(forgedLength));
    changed.append(reinterpret_cast<const char*>(falseSize), sizeof(falseSize));
    COMPRESS_TEST_CHECK(Compress::uncompress(changed).empty(), "forged matching inner and outer lengths rejected");
    const std::string text("a\0b\0c", 5);
    std::string packet = Compress::compress(text);
    const uint32_t textLength = static_cast<uint32_t>(text.size());
    COMPRESS_TEST_CHECK(packet.size() > sizeof(textLength) &&
        std::memcmp(packet.data(), &textLength, sizeof(textLength)) == 0 &&
        Compress::uncompressBound(packet.data()) == textLength, "native-order four-byte length header");
    COMPRESS_TEST_CHECK(Compress::uncompress(packet) == text, "wrapped binary string round-trip");
    std::string limited;
    Compress::uncompress(limited, packet, 5);
    COMPRESS_TEST_CHECK(limited == text, "exact output budget");
    Compress::uncompress(limited, packet, 4);
    COMPRESS_TEST_CHECK(limited.empty(), "output exceeds budget by one");
    limited = packet;
    Compress::uncompress(limited, limited, -1);
    COMPRESS_TEST_CHECK(limited == text, "unlimited in-place decode");
    Compress::uncompress(limited, packet, -2);
    COMPRESS_TEST_CHECK(limited.empty(), "invalid negative budget");
    limited = packet;
    Compress::uncompress(limited, limited, 0);
    COMPRESS_TEST_CHECK(limited.empty(), "zero budget rejects nonempty in-place output");
    limited = text;
    Compress::uncompress(limited, packet.substr(0, 3), 8);
    COMPRESS_TEST_CHECK(limited.empty(), "budgeted decode rejects incomplete header");
    std::string alias = text;
    Compress::compress(alias, alias);
    COMPRESS_TEST_CHECK(Compress::uncompress(alias) == text, "in-place compression produced a valid frame");
    Compress::uncompress(alias, alias);
    COMPRESS_TEST_CHECK(alias == text, "aliased string input/output");
    packet.push_back('x');
    COMPRESS_TEST_CHECK(Compress::uncompress(packet).empty(), "wrapped format rejects trailing bytes");
    COMPRESS_TEST_CHECK(Compress::compress(std::string()).empty(), "wrapped empty string");
    const std::string original(1024, 'q');
    const uint32_t capacity = Compress::compressBound(static_cast<uint32_t>(original.size()));
    COMPRESS_TEST_CHECK(capacity == Compress::rawCompressBound(static_cast<uint32_t>(original.size())) + sizeof(uint32_t),
        "wrapped bound includes exactly the length header");
    std::vector<char> wrapped(capacity + 2, '!');
    uint32_t written = capacity;
    Compress::compress(wrapped.data() + 1, written, original.data(), static_cast<uint32_t>(original.size()));
    COMPRESS_TEST_CHECK(written > 4 && written <= capacity && wrapped.front() == '!' && wrapped.back() == '!', "wrapped output guards");
    const std::string valid(wrapped.data() + 1, written);
    const std::string stringFrame = Compress::compress(original);
    COMPRESS_TEST_CHECK(valid == stringFrame && Compress::uncompress(valid) == original && Compress::uncompress(stringFrame) == original,
        "buffer and string formats interoperate");
    std::vector<char> target(original.size() + 2, '!');
    uint32_t length = static_cast<uint32_t>(original.size());
    Compress::uncompress(target.data() + 1, length, valid.data(), written);
    COMPRESS_TEST_CHECK(length == original.size() && std::memcmp(target.data() + 1, original.data(), length) == 0 &&
        target.front() == '!' && target.back() == '!', "wrapped buffer round trip");
    --length;
    target[length + 1] = '?';
    Compress::uncompress(target.data() + 1, length, valid.data(), written);
    COMPRESS_TEST_CHECK(length == 0 && target.front() == '!' && target[original.size()] == '?' &&
        target.back() == '!', "wrapped short buffer");
    std::string corrupt = valid;
    corrupt.back() ^= 1;
    COMPRESS_TEST_CHECK(Compress::uncompress(corrupt).empty(), "checksum mismatch");
    std::string forged = valid;
    const uint32_t invalidSize = UINT32_MAX;
    std::memcpy(&forged[0], &invalidSize, sizeof(invalidSize));
    COMPRESS_TEST_CHECK(Compress::uncompress(forged).empty(), "forged uncompressed size");
    COMPRESS_TEST_CHECK(Compress::uncompress(valid + valid).empty(), "wrapped extra frame");
    const std::string extraFrame = valid + valid;
    length = static_cast<uint32_t>(original.size());
    Compress::uncompress(target.data() + 1, length, extraFrame.data(), static_cast<uint32_t>(extraFrame.size()));
    COMPRESS_TEST_CHECK(length == 0 && target.front() == '!' && target.back() == '!', "buffer rejects a second frame");
    COMPRESS_TEST_CHECK(Compress::uncompress(valid.substr(0, 3)).empty(), "incomplete length field");
    COMPRESS_TEST_CHECK(Compress::compressBound(UINT32_MAX) == 0 && Compress::uncompressBound(nullptr) == 0, "bounds");
    std::string previous = "previous";
    Compress::compress(previous, original, -1);
    COMPRESS_TEST_CHECK(previous.empty(), "negative level clears output");
    previous = "previous";
    Compress::uncompress(previous, corrupt);
    COMPRESS_TEST_CHECK(previous.empty(), "corrupt input clears output");
    // 首次解压到空字符串和再次复用同一目标都必须得到完整的大帧
    const std::string large(262145, 'L');
    const std::string largeFrame = Compress::compress(large);
    std::string largeOutput;
    Compress::uncompress(largeOutput, largeFrame, large.size());
    COMPRESS_TEST_CHECK(largeOutput == large, "cold destination for known-size large frame");
    largeOutput.assign(large.size(), '!');
    Compress::uncompress(largeOutput, largeFrame);
    COMPRESS_TEST_CHECK(largeOutput == large, "reused destination for known-size large frame");
    uint32_t shortCapacity = sizeof(uint32_t);
    std::memset(wrapped.data(), '!', wrapped.size());
    Compress::compress(wrapped.data() + 1, shortCapacity, original.data(), static_cast<uint32_t>(original.size()));
    COMPRESS_TEST_CHECK(shortCapacity == 0 && wrapped.front() == '!' && wrapped[1 + sizeof(uint32_t)] == '!',
        "header-only compression capacity");
    return 0;
}

// 依稳定编号排列，指定范围时使用同一组名称和回调
static const CompressTestCase kCases[] =
{
    {CompressTestRawLevels, true, "raw-levels", CheckRawLevels},
    {CompressTestRawFrames, false, "raw-frames", CheckRawFrames},
    {CompressTestWrapped, false, "wrapped", CheckWrapped}
};

/** 执行一个已登记的功能组并打印该组的检查数
@param [in] item 本组登记及回调
@return 通过为0，失败为1
*/
static int RunCase(const CompressTestCase& item)
{
    int checks = 0;
    const int result = item.m_run(checks);
    std::printf("{\"checks\":%d,\"bits\":%d}\n", checks, static_cast<int>(sizeof(void*) * 8));
    return result;
}

/** 先运行瞬时组再运行压力组，某组失败后继续下一组
@param [in] reportCases 为true时按组报告，否则输出总检查数
@return 全部通过为0，有失败为1
*/
static int RunAll(bool reportCases)
{
    int checks = 0;
    int failures = 0;
    for (int phase = 0; phase < 2; ++phase)
    {
        for (const CompressTestCase& item : kCases)
        {
            if (item.stress == (phase == 1))
            {
                if (reportCases)
                {
                    const int code = RunCase(item);
                    std::printf("case %u returned %d\n", item.id, code);
                    failures += code;
                }
                else
                {
                    failures += item.m_run(checks);
                }
            }
        }
    }
    if (!reportCases)
    {
        std::printf("{\"checks\":%d,\"bits\":%d}\n", checks, static_cast<int>(sizeof(void*) * 8));
    }
    return failures == 0 ? 0 : 1;
}

extern "C"
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
int runCompressTests()
{
    return RunAll(false);
}

/** 使用公开接口运行固定输入的性能测试
@return 正确性校验通过为0，失败返回非0
*/
static int RunBenchmarks()
{
    std::string source(65536, 'x');
    for (size_t i = 0; i < source.size(); ++i)
    {
        source[i] = static_cast<char>((i * 17 + i / 113 + i / 1024) % 127);
    }
    const std::string reference = Compress::compress(source, 3);
    std::string packed;
    std::string output;
    if (reference.empty() || Compress::uncompress(reference) != source)
    {
        return 1;
    }
    const uint64_t frameBytes = reference.size();
    // 每次压缩同一输入，返回整个批次产生的字节数，复用调用方的字符串
    LibraryBenchmark::run("zstd.string.compress.65536.level3", [&](uint32_t count) -> uint64_t
    {
        uint64_t bytes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            Compress::compress(packed, source, 3);
            bytes += packed.size();
        }
        if (bytes != frameBytes * count)
        {
            throw std::runtime_error("inconsistent compressed batch length");
        }
        return bytes;
    });
    if (Compress::uncompress(packed) != source)
    {
        return 1;
    }
    // count 为本批解压次数，返回总输出字节数；使用独立的固定输入帧
    LibraryBenchmark::run("zstd.string.decompress.65536", [&](uint32_t count) -> uint64_t
    {
        uint64_t bytes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            Compress::uncompress(output, reference);
            bytes += output.size();
        }
        if (bytes != static_cast<uint64_t>(source.size()) * count)
        {
            throw std::runtime_error("incomplete decompression batch");
        }
        return bytes;
    });
    std::printf("plain_bytes=%u compressed_bytes=%u\n", static_cast<unsigned>(source.size()),
        static_cast<unsigned>(packed.size()));
    if (output != source)
    {
        return 1;
    }
    const std::string streamed = MakeUnknownSizeFrame();
    const std::string expected(65537, 'Z');
    // count 为本批限额解压次数，返回总输出字节数，完整内容在计时外核对
    LibraryBenchmark::run("zstd.string.decompress.unknown.rle65537", [&](uint32_t count) -> uint64_t
    {
        uint64_t bytes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            Compress::uncompress(output, streamed, static_cast<int64_t>(expected.size()));
            bytes += output.size();
        }
        if (bytes != static_cast<uint64_t>(expected.size()) * count)
        {
            throw std::runtime_error("incomplete streaming frame");
        }
        return bytes;
    });
    return output == expected ? 0 : 1;
}

/** 解析功能组、数字范围、列表和独立性能模式
@param [in] argc 参数数量
@param [in] argv 命令行参数
@return 通过返回0，断言失败返回1，未知参数返回2
*/
int main(int argc, char* argv[])
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        try
        {
            return RunBenchmarks();
        }
        catch (const std::exception& error)
        {
            std::fprintf(stderr, "Benchmark failed: %s\n", error.what());
            return 1;
        }
    }

    if (argc == 2 && std::strcmp(argv[1], "--list") == 0)
    {
        for (const CompressTestCase& item : kCases)
        {
            std::printf("%s%s", &item == kCases ? "" : " ", item.m_name);
        }
        std::putchar('\n');
        return 0;
    }
    if (argc == 2 && std::strncmp(argv[1], "--case=", 7) == 0)
    {
        for (const CompressTestCase& item : kCases)
        {
            if (std::strcmp(argv[1] + 7, item.m_name) == 0)
            {
                return RunCase(item);
            }
        }
        return 2;
    }
    if (argc <= 1)
    {
        return runCompressTests();
    }
    if (argc == 2 && std::strcmp(argv[1], "--all") == 0)
    {
        return RunAll(true);
    }
    return LibraryCaseSelection::run(argc, argv, kCases, RunCase);
}