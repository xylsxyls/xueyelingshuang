#include "CompressTest.h"
#include "../../../common/PlatformTest/LibraryBenchmark.h"
#include "../../../common/PlatformTest/LibraryNamedCaseSelection.h"
#include "CompressAPI.h"
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
    if (!(value)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, message); return 1; } \
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
            std::vector<uint8_t> packed(capacity + 1);
            for (int level = 0; level <= 9; ++level)
            {
                uint32_t size = capacity;
                COMPRESS_TEST_CHECK(Compress::rawCompress(&packed[0], size, &input[0], length, level) == COMPRESS_OK, "C++ compression");
                uint32_t actual = length, consumed = size;
                COMPRESS_TEST_CHECK(Compress::rawUncompress(&plain[0], actual, &packed[0], consumed) == COMPRESS_OK, "C++ decompression");
                COMPRESS_TEST_CHECK(actual == length && consumed == size && std::memcmp(&plain[0], &input[0], length) == 0, "round-trip bytes and lengths");
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
    COMPRESS_TEST_CHECK(Compress::rawCompress(packed, size, nullptr, 1) == COMPRESS_INVALID_ARGUMENT && size == 0, "null source");
    size = sizeof(packed);
    COMPRESS_TEST_CHECK(Compress::rawCompress(nullptr, size, input, 1) == COMPRESS_INVALID_ARGUMENT && size == 0, "null output");
    size = 1;
    COMPRESS_TEST_CHECK(Compress::rawCompress(packed, size, input, sizeof(input)) == COMPRESS_BUFFER_TOO_SMALL && size == 0, "small compression buffer");
    size = sizeof(packed);
    COMPRESS_TEST_CHECK(Compress::rawCompress(packed, size, input, sizeof(input)) == COMPRESS_OK, "valid stream for negative cases");
    const uint32_t packedSize = size;
    uint32_t actual = sizeof(output), consumed = packedSize + 1;
    packed[packedSize] = 42;
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, packed, consumed) == COMPRESS_OK && consumed == packedSize, "trailing bytes reported to C++ caller");
    actual = 1;
    consumed = packedSize;
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, packed, consumed) == COMPRESS_BUFFER_TOO_SMALL && actual == 0 && consumed == 0, "small decompression buffer");
    actual = sizeof(output);
    consumed = packedSize - 1;
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, packed, consumed) == COMPRESS_DATA_ERROR && actual == 0 && consumed == 0, "truncated stream");
    packed[0] ^= 255;
    actual = sizeof(output);
    consumed = packedSize;
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, packed, consumed) == COMPRESS_DATA_ERROR && actual == 0 && consumed == 0, "corrupt stream");
    // 旧版zlib数据和独立构造的Zstandard原始块分别验证格式切换。
    const uint8_t known[] = {0x78, 0x9c, 0xcb, 0x48, 0xcd, 0xc9, 0xc9, 0x07, 0x00, 0x06, 0x2c, 0x02, 0x15};
    actual = sizeof(output);
    consumed = sizeof(known);
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, known, consumed) == COMPRESS_DATA_ERROR && actual == 0 && consumed == 0, "old zlib format rejected");
    const uint8_t frame[] = {0x28, 0xb5, 0x2f, 0xfd, 0x20, 0x05, 0x29, 0x00, 0x00, 'h', 'e', 'l', 'l', 'o'};
    actual = sizeof(output);
    consumed = sizeof(frame);
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, frame, consumed) == COMPRESS_OK &&
        actual == 5 && consumed == sizeof(frame) && std::memcmp(output, "hello", 5) == 0, "independent Zstandard frame");
    uint8_t skip[] = {0x50, 0x2a, 0x4d, 0x18, 0, 0, 0, 0};
    actual = sizeof(output);
    consumed = sizeof(skip);
    COMPRESS_TEST_CHECK(Compress::rawUncompress(output, actual, skip, consumed) == COMPRESS_DATA_ERROR, "skippable frame rejected");
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
    // 单段帧头谎报4 GiB，但正文只有一个字节，不能按声明分配大缓冲区。
    const uint8_t falseSize[] = {0x28, 0xb5, 0x2f, 0xfd, 0xa0, 0xff, 0xff, 0xff, 0xff, 9, 0, 0, 'Z'};
    changed.assign(sizeof(forgedLength), '\0');
    std::memcpy(&changed[0], &forgedLength, sizeof(forgedLength));
    changed.append(reinterpret_cast<const char*>(falseSize), sizeof(falseSize));
    COMPRESS_TEST_CHECK(Compress::uncompress(changed).empty(), "forged matching inner and outer lengths rejected");
    const std::string text("a\0b\0c", 5);
    std::string legacy = Compress::compress(text);
    COMPRESS_TEST_CHECK(Compress::uncompressBound(legacy.data()) == text.size(), "historical four-byte length header");
    COMPRESS_TEST_CHECK(Compress::uncompress(legacy) == text, "historical binary string round-trip");
    std::string limited;
    Compress::uncompress(limited, legacy, 5);
    COMPRESS_TEST_CHECK(limited == text, "exact output budget");
    Compress::uncompress(limited, legacy, 4);
    COMPRESS_TEST_CHECK(limited.empty(), "output exceeds budget by one");
    limited = legacy;
    Compress::uncompress(limited, limited, -1);
    COMPRESS_TEST_CHECK(limited == text, "unlimited in-place decode");
    Compress::uncompress(limited, legacy, -2);
    COMPRESS_TEST_CHECK(limited.empty(), "invalid negative budget");
    limited = legacy;
    Compress::uncompress(limited, limited, 0);
    COMPRESS_TEST_CHECK(limited.empty(), "zero budget rejects nonempty in-place output");
    limited = text;
    Compress::uncompress(limited, legacy.substr(0, 3), 8);
    COMPRESS_TEST_CHECK(limited.empty(), "budgeted decode rejects incomplete header");
    std::string alias = text;
    Compress::compress(alias, alias);
    Compress::uncompress(alias, alias);
    COMPRESS_TEST_CHECK(alias == text, "aliased string input/output");
    legacy.push_back('x');
    COMPRESS_TEST_CHECK(Compress::uncompress(legacy).empty(), "historical format rejects trailing bytes");
    COMPRESS_TEST_CHECK(Compress::compress(std::string()).empty(), "historical empty string");
    const std::string original(1024, 'q');
    const uint32_t capacity = Compress::compressBound(static_cast<uint32_t>(original.size()));
    std::vector<char> wrapped(capacity + 2, '!');
    uint32_t written = capacity;
    Compress::compress(wrapped.data() + 1, written, original.data(), static_cast<uint32_t>(original.size()));
    COMPRESS_TEST_CHECK(written > 4 && written <= capacity && wrapped.front() == '!' && wrapped.back() == '!', "wrapped output guards");
    const std::string valid(wrapped.data() + 1, written);
    COMPRESS_TEST_CHECK(valid == Compress::compress(original), "buffer and string agree");
    std::vector<char> target(original.size() + 2, '!');
    uint32_t length = static_cast<uint32_t>(original.size());
    Compress::uncompress(target.data() + 1, length, valid.data(), written);
    COMPRESS_TEST_CHECK(length == original.size() && std::memcmp(target.data() + 1, original.data(), length) == 0 &&
        target.front() == '!' && target.back() == '!', "wrapped buffer round trip");
    --length;
    Compress::uncompress(target.data() + 1, length, valid.data(), written);
    COMPRESS_TEST_CHECK(length == 0 && target.front() == '!' && target.back() == '!', "wrapped short buffer");
    std::string corrupt = valid;
    corrupt.back() ^= 1;
    COMPRESS_TEST_CHECK(Compress::uncompress(corrupt).empty(), "checksum mismatch");
    std::string forged = valid;
    const uint32_t invalidSize = UINT32_MAX;
    std::memcpy(&forged[0], &invalidSize, sizeof(invalidSize));
    COMPRESS_TEST_CHECK(Compress::uncompress(forged).empty(), "forged uncompressed size");
    COMPRESS_TEST_CHECK(Compress::uncompress(valid + valid).empty(), "wrapped extra frame");
    COMPRESS_TEST_CHECK(Compress::uncompress(valid.substr(0, 3)).empty(), "incomplete length field");
    COMPRESS_TEST_CHECK(Compress::compressBound(UINT32_MAX) == 0 && Compress::uncompressBound(nullptr) == 0, "bounds");
    std::string previous = "previous";
    Compress::compress(previous, original, -1);
    COMPRESS_TEST_CHECK(previous.empty(), "negative level clears output");
    previous = "previous";
    Compress::uncompress(previous, corrupt);
    COMPRESS_TEST_CHECK(previous.empty(), "corrupt input clears output");
    return 0;
}

/** 运行全部或一个指定的压缩功能用例
@param [in] name 公开用例ID，空指针运行全部
@return 通过为0，断言失败为1，用例未知为2
*/
static int RunSelected(const char* name)
{
    int checks = 0;
    if (name == nullptr || std::strcmp(name, "raw-levels") == 0)
    {
        if (CheckRawLevels(checks) != 0)
        {
            return 1;
        }
    }
    if (name == nullptr || std::strcmp(name, "raw-frames") == 0)
    {
        if (CheckRawFrames(checks) != 0)
        {
            return 1;
        }
    }
    if (name == nullptr || std::strcmp(name, "wrapped") == 0)
    {
        if (CheckWrapped(checks) != 0)
        {
            return 1;
        }
    }
    std::printf("{\"checks\":%d,\"bits\":%d}\n", checks, static_cast<int>(sizeof(void*) * 8));
    return checks == 0 ? 2 : 0;
}

extern "C"
#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE
#endif
int runCompressTests()
{
    return RunSelected(nullptr);
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
    std::string packed = Compress::compress(source, 3);
    std::string output;
    if (packed.empty() || Compress::uncompress(packed) != source)
    {
        return 1;
    }
    const uint64_t frameBytes = packed.size();
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
    // 复用目标字符串解压同一完整帧，计时区外核对全部原始字节
    LibraryBenchmark::run("zstd.string.decompress.65536", [&](uint32_t count) -> uint64_t
    {
        uint64_t bytes = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            Compress::uncompress(output, packed);
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

/** 默认运行全部功能组，可选择一个ID、列出选项或单独运行性能模式
@param [in] argc 参数数量
@param [in] argv 命令行参数
@return 通过返回0，断言失败返回1，未知参数返回2
*/
static int RunSuite(int argc, char* argv[])
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        return RunBenchmarks();
    }

    if (argc == 2 && std::strcmp(argv[1], "--list") == 0)
    {
        std::puts("raw-levels raw-frames wrapped");
        return 0;
    }
    if (argc == 2 && std::strncmp(argv[1], "--case=", 7) == 0)
    {
        return RunSelected(argv[1] + 7);
    }
    return argc <= 1 ? runCompressTests() : 2;
}

/** 执行所选功能组，默认运行完整套件。
@param [in] argc 命令行参数数量
@param [in] argv 命令行参数数组
@return 0 表示通过，1 表示断言失败，2 表示参数无效
*/
int main(int argc, char* argv[])
{
    const LibraryNamedTestCase cases[] =
    {
        {CompressTestRawLevels, true, "raw-levels"},
        {CompressTestRawFrames, false, "raw-frames"},
        {CompressTestWrapped, false, "wrapped"},
    };
    return LibraryCaseSelection::runNamed(argc, argv, cases, RunSuite);
}