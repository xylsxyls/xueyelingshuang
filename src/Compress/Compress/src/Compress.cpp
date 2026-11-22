#include "Compress.h"
#include "zstandard/zstd.h"
#include "zstandard/zstd_errors.h"
#include <cstring>
#include <memory>

#if defined(_MSC_VER) && !defined(COMPRESS_NO_AUTOLINK)
#if defined(_DEBUG)
#pragma comment(lib, "zstdd.lib")
#else
#pragma comment(lib, "zstd.lib")
#endif
#endif

// 封装帧的本机字节序原长字段宽度
static const uint32_t kCompressLengthSize = sizeof(uint32_t);
// 标准数据帧的固定前缀，排除可跳过帧
static const uint8_t kFrameMagic[] = {0x28, 0xb5, 0x2f, 0xfd};

/** 将Zstandard返回的错误转换为公开接口错误
@param [in] result 已由ZSTD_isError判断为失败的结果
@param [in] fallback 其余错误对应的公开错误码
@return CompressResult中的错误码
*/
static int32_t CompressError(size_t result, int32_t fallback)
{
    switch (ZSTD_getErrorCode(result))
    {
    case ZSTD_error_dstSize_tooSmall:
        return COMPRESS_BUFFER_TOO_SMALL;
    case ZSTD_error_memory_allocation:
        return COMPRESS_NO_MEMORY;
    default:
        return fallback;
    }
}

uint32_t Compress::rawCompressBound(uint32_t srcLength)
{
    const size_t bound = ZSTD_compressBound(srcLength);
    if (ZSTD_isError(bound) || bound < srcLength || bound > UINT32_MAX)
    {
        return 0;
    }
    return static_cast<uint32_t>(bound);
}

int32_t Compress::rawCompress(uint8_t* dest, uint32_t& destLength, const uint8_t* src, uint32_t srcLength, int32_t level)
{
    const uint32_t capacity = destLength;
    destLength = 0;
    if (dest == nullptr || src == nullptr || level < 0 || level > 9)
    {
        return COMPRESS_INVALID_ARGUMENT;
    }
    ZSTD_CCtx* context = ZSTD_createCCtx();
    if (context == nullptr)
    {
        return COMPRESS_NO_MEMORY;
    }
    size_t result = ZSTD_CCtx_setParameter(context, ZSTD_c_compressionLevel, level);
    if (!ZSTD_isError(result))
    {
        result = ZSTD_CCtx_setParameter(context, ZSTD_c_checksumFlag, 1);
    }
    if (!ZSTD_isError(result))
    {
        result = ZSTD_compress2(context, dest, capacity, src, srcLength);
    }
    ZSTD_freeCCtx(context);
    if (ZSTD_isError(result))
    {
        return CompressError(result, COMPRESS_INVALID_ARGUMENT);
    }
    destLength = static_cast<uint32_t>(result);
    return COMPRESS_OK;
}

int32_t Compress::rawUncompress(uint8_t* dest, uint32_t& destLength, const uint8_t* src, uint32_t& srcLength)
{
    const uint32_t capacity = destLength;
    const uint32_t available = srcLength;
    destLength = 0;
    srcLength = 0;
    if (dest == nullptr || src == nullptr || &destLength == &srcLength)
    {
        return COMPRESS_INVALID_ARGUMENT;
    }
    // 网络协议只接受标准数据帧，解出首帧后将消费长度交回上层。
    if (available < sizeof(kFrameMagic) || std::memcmp(src, kFrameMagic, sizeof(kFrameMagic)) != 0)
    {
        return COMPRESS_DATA_ERROR;
    }
    const size_t frameSize = ZSTD_findFrameCompressedSize(src, available);
    if (ZSTD_isError(frameSize))
    {
        return COMPRESS_DATA_ERROR;
    }
    const size_t result = ZSTD_decompress(dest, capacity, src, frameSize);
    if (ZSTD_isError(result))
    {
        return CompressError(result, COMPRESS_DATA_ERROR);
    }
    destLength = static_cast<uint32_t>(result);
    srcLength = static_cast<uint32_t>(frameSize);
    return COMPRESS_OK;
}

void Compress::compress(char* dest, uint32_t& destLength, const char* src, uint32_t srcLength, int32_t level)
{
    const uint32_t capacity = destLength;
    destLength = 0;
    if (dest == nullptr || src == nullptr || srcLength == 0 || capacity <= kCompressLengthSize)
    {
        return;
    }
    uint32_t length = capacity - kCompressLengthSize;
    if (Compress::rawCompress(reinterpret_cast<uint8_t*>(dest) + kCompressLengthSize, length,
        reinterpret_cast<const uint8_t*>(src), srcLength, level) != COMPRESS_OK)
    {
        return;
    }
    std::memcpy(dest, &srcLength, sizeof(srcLength));
    destLength = length + kCompressLengthSize;
}

void Compress::compress(std::string& dest, const std::string& src, int32_t level)
{
    if (src.empty() || src.size() > UINT32_MAX || level < 0 || level > 9)
    {
        dest.clear();
        return;
    }
    const uint32_t length = static_cast<uint32_t>(src.size());
    const uint32_t capacity = Compress::compressBound(length);
    if (capacity == 0)
    {
        dest.clear();
        return;
    }
    // 只有原地压缩才需要独立输出，分离的目标可以沿用上一帧的容量
    std::string temporary;
    std::string& packed = &dest == &src ? temporary : dest;
    packed.resize(capacity);
    uint32_t actual = capacity;
    Compress::compress(&packed[0], actual, src.data(), length, level);
    if (actual == 0)
    {
        dest.clear();
        return;
    }
    packed.resize(actual);
    if (&packed != &dest)
    {
        dest.swap(packed);
    }
}

void Compress::uncompress(std::string& dest, const std::string& src)
{
    if (src.size() <= kCompressLengthSize || src.size() > UINT32_MAX)
    {
        dest.clear();
        return;
    }
    const uint32_t expected = Compress::uncompressBound(src.data());
    const char* frame = src.data() + kCompressLengthSize;
    const size_t frameSize = src.size() - kCompressLengthSize;
    // 完整帧可以省略内容长度；长度头与实际解码字节数始终需要一致。
    const unsigned long long declared = ZSTD_getFrameContentSize(frame, frameSize);
    if (frameSize < sizeof(kFrameMagic) || std::memcmp(frame, kFrameMagic, sizeof(kFrameMagic)) != 0 ||
        (declared != ZSTD_CONTENTSIZE_UNKNOWN && declared != expected) ||
        ZSTD_findFrameCompressedSize(frame, frameSize) != frameSize)
    {
        dest.clear();
        return;
    }
    std::string temporary;
    std::string& plain = &dest == &src ? temporary : dest;
    if (declared != ZSTD_CONTENTSIZE_UNKNOWN &&
        (expected <= plain.capacity() || expected <= ZSTD_DStreamOutSize()))
    {
        // 已有容量或小帧直接解码，避免常见调用的分块开销。
        plain.resize(expected);
        uint8_t empty = 0;
        void* bytes = expected == 0 ? static_cast<void*>(&empty) : &plain[0];
        const size_t actual = ZSTD_decompress(bytes, expected, frame, frameSize);
        if (ZSTD_isError(actual) || actual != expected)
        {
            dest.clear();
            return;
        }
    }
    else
    {
        // 未知原长或大幅扩容时只按实际产生的字节增长，避免先按伪造的长度分配。
        std::unique_ptr<ZSTD_DStream, decltype(&ZSTD_freeDStream)> stream(ZSTD_createDStream(), &ZSTD_freeDStream);
        if (!stream || ZSTD_isError(ZSTD_initDStream(stream.get())))
        {
            dest.clear();
            return;
        }
        ZSTD_inBuffer input = {frame, frameSize, 0};
        char chunk[8192];
        plain.clear();
        size_t remaining = 1;
        while (remaining != 0)
        {
            ZSTD_outBuffer output = {chunk, sizeof(chunk), 0};
            const size_t before = input.pos;
            remaining = ZSTD_decompressStream(stream.get(), &output, &input);
            if (ZSTD_isError(remaining) || output.pos > expected - plain.size() ||
                (remaining != 0 && output.pos == 0 && before == input.pos))
            {
                dest.clear();
                return;
            }
            plain.append(chunk, output.pos);
        }
        if (plain.size() != expected || input.pos != frameSize)
        {
            dest.clear();
            return;
        }
    }
    if (&plain != &dest)
    {
        dest.swap(plain);
    }
}

void Compress::uncompress(std::string& dest, const std::string& src, int64_t maxOutputBytes)
{
    if (maxOutputBytes < -1 || src.size() <= kCompressLengthSize ||
        (maxOutputBytes >= 0 && Compress::uncompressBound(src.data()) > maxOutputBytes))
    {
        dest.clear();
        return;
    }
    Compress::uncompress(dest, src);
}

uint32_t Compress::compressBound(uint32_t srcLength)
{
    const uint32_t length = Compress::rawCompressBound(srcLength);
    if (length == 0 || length > UINT32_MAX - kCompressLengthSize)
    {
        return 0;
    }
    return length + kCompressLengthSize;
}

uint32_t Compress::uncompressBound(const char* src)
{
    uint32_t length = 0;
    if (src != nullptr)
    {
        std::memcpy(&length, src, sizeof(length));
    }
    return length;
}

void Compress::uncompress(char* dest, uint32_t& destLength, const char* src, uint32_t srcLength)
{
    const uint32_t capacity = destLength;
    destLength = 0;
    if (dest == nullptr || src == nullptr || srcLength <= kCompressLengthSize)
    {
        return;
    }
    const uint32_t expected = Compress::uncompressBound(src);
    if (expected > capacity)
    {
        return;
    }
    uint32_t actual = capacity;
    uint32_t consumed = srcLength - kCompressLengthSize;
    if (Compress::rawUncompress(reinterpret_cast<uint8_t*>(dest), actual,
        reinterpret_cast<const uint8_t*>(src) + kCompressLengthSize, consumed) != COMPRESS_OK ||
        actual != expected || consumed != srcLength - kCompressLengthSize)
    {
        return;
    }
    destLength = actual;
}

std::string Compress::compress(const std::string& src, int32_t level)
{
    std::string result;
    Compress::compress(result, src, level);
    return result;
}

std::string Compress::uncompress(const std::string& src)
{
    std::string result;
    Compress::uncompress(result, src);
    return result;
}

// 使用示例（独立消费者）
//#include "Compress/CompressAPI.h"
//#include <cstdio>
//#include <vector>
//
//int main()
//{
//    const std::string source("binary\0sample", 13);
//    // 上层已有报文头时传原始Zstandard帧，并核对解压消耗的输入字节数
//    std::vector<uint8_t> frame(Compress::rawCompressBound(static_cast<uint32_t>(source.size())));
//    uint32_t size = static_cast<uint32_t>(frame.size());
//    if (Compress::rawCompress(frame.data(), size, reinterpret_cast<const uint8_t*>(source.data()),
//        static_cast<uint32_t>(source.size()), 3) != COMPRESS_OK)
//    {
//        return 1;
//    }
//    std::vector<uint8_t> output(source.size());
//    uint32_t capacity = static_cast<uint32_t>(output.size());
//    uint32_t consumed = size;
//    if (Compress::rawUncompress(output.data(), capacity, frame.data(), consumed) != COMPRESS_OK ||
//        capacity != source.size() || consumed != size || std::string(output.begin(), output.end()) != source)
//    {
//        return 2;
//    }
//    // 带原长头的接口可直接使用字符串或由应用提供缓冲
//    std::string packed = Compress::compress(source);
//    if (Compress::uncompress(packed) != source)
//    {
//        return 3;
//    }
//    Compress::compress(packed, source, 3);
//    std::string decoded;
//    Compress::uncompress(decoded, packed);
//    if (decoded != source)
//    {
//        return 4;
//    }
//    Compress::uncompress(packed, packed, static_cast<int64_t>(source.size()));
//    if (packed != source)
//    {
//        return 4;
//    }
//    std::vector<char> buffer(Compress::compressBound(static_cast<uint32_t>(source.size())));
//    size = static_cast<uint32_t>(buffer.size());
//    Compress::compress(buffer.data(), size, source.data(), static_cast<uint32_t>(source.size()));
//    if (size == 0 || Compress::uncompressBound(buffer.data()) != source.size())
//    {
//        return 5;
//    }
//    std::vector<char> plain(source.size());
//    capacity = static_cast<uint32_t>(plain.size());
//    Compress::uncompress(plain.data(), capacity, buffer.data(), size);
//    std::printf("original=%u framed=%u\n", static_cast<unsigned>(capacity), static_cast<unsigned>(size));
//    return capacity == source.size() && std::string(plain.begin(), plain.end()) == source ? 0 : 6;
//}