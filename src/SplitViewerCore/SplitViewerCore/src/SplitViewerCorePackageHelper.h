#pragma once
#include "SplitViewerCoreConfig.h"
#include <stdint.h>
#include <stddef.h>
#include <vector>

/** PNG配置块和旧版尾部配置的封装编解码，处理块结构、配置CRC和字节预算，不持有文件资源
*/
class SplitViewerCorePackageHelper
{
public:
    /** 追加大端无符号32位整数
    @param [in,out] bytes 字节缓冲区
    @param [in] value 要以大端四字节追加的无符号整数
    */
    static void appendUInt32(std::vector<uint8_t>& bytes, uint32_t value);

    /** 读取大端32位整数；调用方须保证至少剩余四字节
    @param [in] bytes 字节缓冲区
    @param [in] index 起始字节偏移，须满足index加4不超过bytes.size()
    @return 由连续四字节按网络字节序组合的无符号整数
    */
    static uint32_t readUInt32(const std::vector<uint8_t>& bytes, size_t index);

    /** 累计更新CRC32校验值
    @param [in] crc 累计校验值
    @param [in] data 借用的字节缓冲区，size大于0时不得为空且须至少包含size字节
    @param [in] size data可读取的字节数，0时不读取data
    @return 未进行最终异或的累计CRC32，供调用方继续累积或结束校验
    */
    static uint32_t crc32(uint32_t crc, const uint8_t* data, size_t size);

    /** 计算PNG块类型和数据的CRC32
    @param [in] type 借用的四字节PNG块类型，不得为空
    @param [in] data 借用的数据，可为空；空指针或size为0时只累计块类型
    @param [in] size data可读取的字节数，0时不读取data
    @return 块类型和数据按PNG协议计算的最终CRC32
    */
    static uint32_t chunkCrc(const uint8_t* type, const uint8_t* data, size_t size);

    /** 检查完整的PNG文件签名
    @param [in] bytes 字节缓冲区
    @return 检查完整的PNG文件签名是否成功
    */
    static bool startsWithPng(const std::vector<uint8_t>& bytes);

    /** 追加PNG块的长度、类型、数据和CRC
    @param [in,out] bytes 字节缓冲区
    @param [in] type 借用的四字节PNG块类型，空指针时拒绝追加
    @param [in] data 待追加的块数据，可为空容器；长度须能用32位无符号数表示
    @return 追加PNG块的长度、类型、数据和CRC是否成功
    */
    static bool appendChunk(std::vector<uint8_t>& bytes, const uint8_t* type, const std::vector<uint8_t>& data);

    /** 在PNG结束块前插入配置块；允许输入输出为同一容器，失败保留输出
    @param [in] thumbnailPng PNG预览字节
    @param [in] configBytes UTF-16LE配置字节
    @param [in,out] packageBytes 封装结果
    @param [in] config 本次调用的实例配置快照，非法配置返回false
    @return 在PNG结束块前插入配置块；允许输入输出为同一容器，失败保留输出是否成功
    */
    static bool buildConfigPackage(const std::vector<uint8_t>& thumbnailPng, const std::vector<uint8_t>& configBytes,
        std::vector<uint8_t>& packageBytes, const SplitViewerCoreConfig& config);

    /** 从PNG配置块或旧版尾部标记提取配置；允许原位提取，失败保留输出
    @param [in] bytes 字节缓冲区
    @param [in,out] configBytes UTF-16LE配置字节
    @param [in] config 本次调用的实例配置快照，非法配置返回false
    @return 从PNG配置块或旧版尾部标记提取配置；允许原位提取，失败保留输出是否成功
    */
    static bool extractEmbeddedConfig(
        const std::vector<uint8_t>& bytes, std::vector<uint8_t>& configBytes, const SplitViewerCoreConfig& config);
};