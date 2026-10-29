#include "Compress.h"
extern "C"
{
#include "zlib/zlib.h"
}
#include <cstring>
#if defined(_MSC_VER) && !defined(COMPRESS_NO_AUTOLINK)
#if defined(_WIN64) && defined(_DEBUG)
#pragma comment(lib, "zlibstatic64d.lib")
#elif defined(_WIN64)
#pragma comment(lib, "zlibstatic64.lib")
#elif defined(_DEBUG)
#pragma comment(lib, "zlibstatic32d.lib")
#else
#pragma comment(lib, "zlibstatic32.lib")
#endif
#endif

static const uint32_t kCompressLengthSize = sizeof(uint32_t);

uint32_t Compress::zlibRawCompressBound(uint32_t srcLength)
{
    // 新版zlib可能以ULONG_MAX表示上界饱和，不能将它当作可保证的容量
    const uLong bound = ::compressBound(srcLength);
    if (bound < srcLength || bound >= UINT32_MAX)
    {
        return 0;
    }
    return static_cast<uint32_t>(bound);
}

int32_t Compress::zlibRawCompress(uint8_t* dest, uint32_t& destLength, const uint8_t* src, uint32_t srcLength, int32_t level)
{
    uLongf length = destLength;
    destLength = 0;
    if (dest == nullptr || src == nullptr || level < 0 || level > 9)
    {
        return COMPRESS_INVALID_ARGUMENT;
    }
    const int result = ::compress2(dest, &length, src, srcLength, level);
    if (result == Z_MEM_ERROR)
    {
        return COMPRESS_NO_MEMORY;
    }
    if (result == Z_BUF_ERROR)
    {
        return COMPRESS_BUFFER_TOO_SMALL;
    }
    if (result != Z_OK)
    {
        return COMPRESS_INVALID_ARGUMENT;
    }
    destLength = static_cast<uint32_t>(length);
    return COMPRESS_OK;
}

int32_t Compress::zlibRawUnCompress(uint8_t* dest, uint32_t& destLength, const uint8_t* src, uint32_t& srcLength)
{
    uLongf length = destLength;
    uLong consumed = srcLength;
    destLength = 0;
    srcLength = 0;
    if (dest == nullptr || src == nullptr)
    {
        return COMPRESS_INVALID_ARGUMENT;
    }
    const int result = ::uncompress2(dest, &length, src, &consumed);
    if (result == Z_MEM_ERROR)
    {
        return COMPRESS_NO_MEMORY;
    }
    if (result == Z_BUF_ERROR)
    {
        return COMPRESS_BUFFER_TOO_SMALL;
    }
    if (result != Z_OK)
    {
        return COMPRESS_DATA_ERROR;
    }
    destLength = static_cast<uint32_t>(length);
    srcLength = static_cast<uint32_t>(consumed);
    return COMPRESS_OK;
}

void Compress::zlibCompress(char* dest, uint32_t& destLength, const char* src, uint32_t srcLength, int32_t level)
{
    const uint32_t capacity = destLength;
    destLength = 0;
    if (dest == nullptr || src == nullptr || srcLength == 0 || capacity <= kCompressLengthSize)
    {
        return;
    }
    uint32_t length = capacity - kCompressLengthSize;
    if (zlibRawCompress(reinterpret_cast<uint8_t*>(dest) + kCompressLengthSize, length,
        reinterpret_cast<const uint8_t*>(src), srcLength, level) != COMPRESS_OK)
    {
        return;
    }
    std::memcpy(dest, &srcLength, sizeof(srcLength));
    destLength = length + kCompressLengthSize;
}

void Compress::zlibCompress(std::string& dest, const std::string& src, int32_t level)
{
    if (src.empty() || src.size() > UINT32_MAX || level < 0 || level > 9)
    {
        dest.clear();
        return;
    }
    const uint32_t length = static_cast<uint32_t>(src.size());
    const uint32_t capacity = zlibCompressBound(length);
    if (capacity == 0)
    {
        dest.clear();
        return;
    }
    // 延迟提交结果，使输入和输出引用同一个字符串时也能安全处理
    std::string packed(capacity, '\0');
    uint32_t actual = capacity;
    zlibCompress(&packed[0], actual, src.data(), length, level);
    if (actual == 0)
    {
        dest.clear();
        return;
    }
    packed.resize(actual);
    dest.swap(packed);
}

void Compress::zlibUnCompress(std::string& dest, const std::string& src)
{
    if (src.size() <= kCompressLengthSize || src.size() > UINT32_MAX)
    {
        dest.clear();
        return;
    }
    const uint32_t expected = zlibUnCompressBound(src.data());
    std::string plain(expected, '\0');
    uint8_t empty = 0;
    uint8_t* bytes = expected == 0 ? &empty : reinterpret_cast<uint8_t*>(&plain[0]);
    uint32_t actual = expected;
    uint32_t consumed = static_cast<uint32_t>(src.size()) - kCompressLengthSize;
    const int32_t result = zlibRawUnCompress(bytes, actual,
        reinterpret_cast<const uint8_t*>(src.data()) + kCompressLengthSize, consumed);
    if (result != COMPRESS_OK || actual != expected || consumed != src.size() - kCompressLengthSize)
    {
        dest.clear();
        return;
    }
    dest.swap(plain);
}

uint32_t Compress::zlibCompressBound(uint32_t srcLength)
{
    const uint32_t length = zlibRawCompressBound(srcLength);
    if (length == 0 || length > UINT32_MAX - kCompressLengthSize)
    {
        return 0;
    }
    return length + kCompressLengthSize;
}

uint32_t Compress::zlibUnCompressBound(const char* src)
{
    uint32_t length = 0;
    if (src != nullptr)
    {
        std::memcpy(&length, src, sizeof(length));
    }
    return length;
}

void Compress::zlibUnCompress(char* dest, uint32_t& destLength, const char* src, uint32_t srcLength)
{
    const uint32_t capacity = destLength;
    destLength = 0;
    if (dest == nullptr || src == nullptr || srcLength <= kCompressLengthSize)
    {
        return;
    }
    const uint32_t expected = zlibUnCompressBound(src);
    if (expected > capacity)
    {
        return;
    }
    uint32_t actual = capacity;
    uint32_t consumed = srcLength - kCompressLengthSize;
    if (zlibRawUnCompress(reinterpret_cast<uint8_t*>(dest), actual,
        reinterpret_cast<const uint8_t*>(src) + kCompressLengthSize, consumed) != COMPRESS_OK ||
        actual != expected || consumed != srcLength - kCompressLengthSize)
    {
        return;
    }
    destLength = actual;
}

std::string Compress::zlibCompress(const std::string& src, int32_t level)
{
    std::string result;
    zlibCompress(result, src, level);
    return result;
}

std::string Compress::zlibUnCompress(const std::string& src)
{
    std::string result;
    zlibUnCompress(result, src);
    return result;
}

//int main()
//{
//	/*原始数据*/
//	unsigned char strsrc[] = "这些是测试数据。123456789 abcdefghigklmnopqrstuvwxyz\n\t\0abcdefghijklmnopqrstuvwxyz\n"; //包含\0字符
//	unsigned char buf[1024] = { 0 };
//	unsigned char strdst[1024] = { 0 };
//	unsigned long srclen = sizeof(strsrc);
//	unsigned long buflen = sizeof(buf);
//	unsigned long dstlen = sizeof(strdst);
//	int i;
//	FILE * fp;
//
//	printf("源串:");
//	for (i = 0; i < srclen; ++i)
//	{
//		printf("%c", strsrc[i]);
//	}
//	printf("原串长度为:%ld\n", srclen);
//
//	printf("字符串预计算长度为:%ld\n", compressBound(srclen));
//	//压缩
//	compress(buf, &buflen, strsrc, srclen);
//	printf("压缩后实际长度为:%ld\n", buflen);
//	dstlen = 1024;
//	//解压缩
//	uncompress(strdst, &dstlen, buf, buflen);
//
//	printf("目的串:");
//	for (i = 0; i < dstlen; ++i)
//	{
//		printf("%c", strdst[i]);
//	}
//
//	getchar();
//	return 0;
//}

// Compress回归测试：测试时取消下面逐行注释，完成后恢复注释
//#include <cstdio>
//#include <vector>
//#if defined(COMPRESS_MYSQL_COEXIST_TEST)
//#include "MysqlCpp/MysqlCppAPI.h"
//#endif
//
//#define COMPRESS_CHECK(value) { if (!(value)) { std::printf("FAIL line=%d\n", __LINE__); return 1; } }
//
//int main()
//{
//#if defined(COMPRESS_MYSQL_COEXIST_TEST)
//    // 只初始化本地驱动，不连接数据库；MySQL经DLL隔离，压缩使用tools预编译zlib
//    MysqlCpp database;
//    COMPRESS_CHECK(database.init());
//#endif
//    // 容量包含四字节长度头，输出不足时不能写入容量之外的哨兵
//    uint8_t zeroes[1024] = {};
//    std::vector<char> tight(10, 0x5a);
//    uint32_t tightLength = 6;
//    Compress::zlibCompress(tight.data(), tightLength, reinterpret_cast<const char*>(zeroes), sizeof(zeroes), 1);
//    for (size_t index = 6; index < tight.size(); ++index)
//    {
//        COMPRESS_CHECK(tight[index] == 0x5a);
//    }
//    COMPRESS_CHECK(tightLength == 0);
//
//    const uint32_t sizes[] = {0, 1, 1023, 1024, 65537, 1048576};
//    const int32_t levels[] = {0, 1, 7, 9};
//    uint32_t rawCases = 0;
//    for (uint32_t i = 0; i < 6; ++i)
//    {
//        for (uint32_t j = 0; j < 4; ++j)
//        {
//            std::vector<uint8_t> input(sizes[i] + 1, 0x41);
//            uint32_t random = 0x896f13a5;
//            for (size_t index = 0; j % 2 && index < input.size(); ++index)
//            {
//                random ^= random << 13; random ^= random >> 17; random ^= random << 5;
//                input[index] = static_cast<uint8_t>(random);
//            }
//            const uint32_t capacity = Compress::zlibRawCompressBound(sizes[i]);
//            COMPRESS_CHECK(capacity != 0);
//            std::vector<uint8_t> packed(capacity + 1, 0x5a);
//            uint32_t length = capacity;
//            COMPRESS_CHECK(Compress::zlibRawCompress(packed.data(), length, input.data(), sizes[i], levels[j]) == COMPRESS_OK);
//            COMPRESS_CHECK(packed[capacity] == 0x5a);
//            std::vector<uint8_t> output(sizes[i] + 1, 0x5a);
//            uint32_t actual = sizes[i], consumed = length;
//            COMPRESS_CHECK(Compress::zlibRawUnCompress(output.data(), actual, packed.data(), consumed) == COMPRESS_OK);
//            COMPRESS_CHECK(actual == sizes[i] && consumed == length && std::memcmp(input.data(), output.data(), actual) == 0);
//            COMPRESS_CHECK(output[sizes[i]] == 0x5a);
//            consumed = length + 1; actual = sizes[i];
//            COMPRESS_CHECK(Compress::zlibRawUnCompress(output.data(), actual, packed.data(), consumed) == COMPRESS_OK && consumed == length);
//            consumed = length - 1; actual = sizes[i];
//            COMPRESS_CHECK(Compress::zlibRawUnCompress(output.data(), actual, packed.data(), consumed) == COMPRESS_DATA_ERROR);
//            COMPRESS_CHECK(consumed == 0 && actual == 0);
//            packed[0] ^= 0xff; consumed = length; actual = sizes[i];
//            COMPRESS_CHECK(Compress::zlibRawUnCompress(output.data(), actual, packed.data(), consumed) == COMPRESS_DATA_ERROR);
//            COMPRESS_CHECK(consumed == 0 && actual == 0);
//            ++rawCases;
//        }
//    }
//
//    uint32_t legacyCases = 0;
//    for (uint32_t i = 1; i < 6; ++i)
//    {
//        for (uint32_t pattern = 0; pattern < 2; ++pattern)
//        {
//            std::string input(sizes[i], '\0');
//            uint32_t random = 0x123b59e1;
//            for (size_t index = 0; index < input.size(); ++index)
//            {
//                random ^= random << 13; random ^= random >> 17; random ^= random << 5;
//                input[index] = pattern ? static_cast<char>(random) : static_cast<char>('A' + index % 3);
//            }
//            for (uint32_t j = 0; j < 4; ++j)
//            {
//                const std::string packed = Compress::zlibCompress(input, levels[j]);
//                COMPRESS_CHECK(packed.size() > 4 && Compress::zlibUnCompressBound(packed.data()) == input.size());
//                COMPRESS_CHECK(Compress::zlibUnCompress(packed) == input);
//                std::string reused("previous output");
//                Compress::zlibCompress(reused, input, levels[j]);
//                COMPRESS_CHECK(reused == packed);
//                Compress::zlibUnCompress(reused, packed);
//                COMPRESS_CHECK(reused == input);
//                const uint32_t capacity = Compress::zlibCompressBound(sizes[i]);
//                std::vector<char> buffer(static_cast<size_t>(capacity) + 2, 0x5a);
//                uint32_t actual = capacity;
//                Compress::zlibCompress(buffer.data() + 1, actual, input.data(), sizes[i], levels[j]);
//                COMPRESS_CHECK(actual == packed.size() && std::memcmp(buffer.data() + 1, packed.data(), actual) == 0);
//                COMPRESS_CHECK(buffer.front() == 0x5a && buffer.back() == 0x5a);
//                COMPRESS_CHECK(Compress::zlibUnCompressBound(buffer.data() + 1) == sizes[i]);
//                std::vector<char> plain(static_cast<size_t>(sizes[i]) + 2, 0x5a);
//                uint32_t plainLength = sizes[i];
//                Compress::zlibUnCompress(plain.data() + 1, plainLength, buffer.data() + 1, actual);
//                COMPRESS_CHECK(plainLength == sizes[i] && std::memcmp(plain.data() + 1, input.data(), sizes[i]) == 0);
//                COMPRESS_CHECK(plain.front() == 0x5a && plain.back() == 0x5a);
//                ++legacyCases;
//            }
//        }
//    }
//
//    // 失败时清理输出，旧格式拒绝损坏、截断、尾随字节及原长不匹配
//    const std::string original(1024, 'Z');
//    const std::string valid = Compress::zlibCompress(original);
//    for (uint32_t index = 0; index < 5; ++index)
//    {
//        std::string invalid = valid;
//        if (index == 0)
//        {
//            invalid.back() ^= 1;
//        }
//        if (index == 1)
//        {
//            invalid.resize(invalid.size() - 1);
//        }
//        if (index == 2)
//        {
//            invalid += 'x';
//        }
//        if (index == 3)
//        {
//            invalid.resize(3);
//        }
//        if (index == 4)
//        {
//            const uint32_t wrong = static_cast<uint32_t>(original.size()) - 1;
//            std::memcpy(&invalid[0], &wrong, sizeof(wrong));
//        }
//        std::string output("previous output");
//        Compress::zlibUnCompress(output, invalid);
//        COMPRESS_CHECK(output.empty() && Compress::zlibUnCompress(invalid).empty());
//        std::vector<char> buffer(original.size() + 2, 0x5a);
//        uint32_t actual = static_cast<uint32_t>(original.size());
//        Compress::zlibUnCompress(buffer.data() + 1, actual, invalid.data(), static_cast<uint32_t>(invalid.size()));
//        COMPRESS_CHECK(actual == 0 && buffer.front() == 0x5a && buffer.back() == 0x5a);
//    }
//    std::string alias = original;
//    Compress::zlibCompress(alias, alias);
//    COMPRESS_CHECK(alias == valid);
//    Compress::zlibUnCompress(alias, alias);
//    COMPRESS_CHECK(alias == original);
//    std::string output("previous output");
//    Compress::zlibCompress(output, std::string());
//    COMPRESS_CHECK(output.empty());
//    output = "previous output";
//    Compress::zlibUnCompress(output, std::string());
//    COMPRESS_CHECK(output.empty());
//    output = "previous output";
//    Compress::zlibCompress(output, original, 10);
//    COMPRESS_CHECK(output.empty());
//    COMPRESS_CHECK(Compress::zlibUnCompressBound(nullptr) == 0);
//    COMPRESS_CHECK(Compress::zlibCompressBound(UINT32_MAX) == 0);
//    uint32_t length = 1023;
//    std::vector<char> small(1025, 0x5a);
//    Compress::zlibUnCompress(small.data() + 1, length, valid.data(), static_cast<uint32_t>(valid.size()));
//    COMPRESS_CHECK(length == 0 && small.front() == 0x5a && small[1024] == 0x5a);
//    uint8_t dest[64] = {};
//    length = 1;
//    COMPRESS_CHECK(Compress::zlibRawCompress(dest, length, zeroes, sizeof(zeroes), 1) == COMPRESS_BUFFER_TOO_SMALL && length == 0);
//    length = sizeof(dest);
//    COMPRESS_CHECK(Compress::zlibRawCompress(dest, length, zeroes, sizeof(zeroes), 10) == COMPRESS_INVALID_ARGUMENT && length == 0);
//    length = sizeof(dest);
//    COMPRESS_CHECK(Compress::zlibRawCompress(nullptr, length, zeroes, sizeof(zeroes)) == COMPRESS_INVALID_ARGUMENT && length == 0);
//    COMPRESS_CHECK(Compress::zlibRawCompressBound(UINT32_MAX) == 0);
//    std::printf("PASS Compress main raw=%u legacy=%u; sentinels, invalid data, capacity, alias and overflow\n", rawCases, legacyCases);
//    return 0;
//}
//
//#undef COMPRESS_CHECK
