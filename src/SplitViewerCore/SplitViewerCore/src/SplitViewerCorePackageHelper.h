#pragma once
#include <stdint.h>
#include <stddef.h>
#include <vector>

/** PNG and legacy config packages. */
class SplitViewerCorePackageHelper
{
public:
    /** 追加大端无符号32位整数。
    @param [in,out] bytes 字节缓冲区
    @param [in] value 输入值
    */
    static void appendUInt32(std::vector<uint8_t>& bytes, uint32_t value);
    /** 读取大端32位整数；调用方须保证至少剩余四字节。
    @param [in] bytes 字节缓冲区
    @param [in] index 图层索引或字节偏移
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static uint32_t readUInt32(const std::vector<uint8_t>& bytes, size_t index);
    /** 累计更新CRC32校验值。
    @param [in] crc 累计校验值
    @param [in] data 输入数据
    @param [in] size 数据长度或区域尺寸
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static uint32_t crc32(uint32_t crc, const uint8_t* data, size_t size);
    /** 计算PNG块类型和数据的CRC32。
    @param [in] type 四字节PNG块类型
    @param [in] data 输入数据
    @param [in] size 数据长度或区域尺寸
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static uint32_t chunkCrc(const uint8_t* type, const uint8_t* data, size_t size);
    /** 检查完整的PNG文件签名。
    @param [in] bytes 字节缓冲区
    @return 是否满足条件或操作成功。
    */
    static bool startsWithPng(const std::vector<uint8_t>& bytes);
    /** 追加PNG块的长度、类型、数据和CRC。
    @param [in,out] bytes 字节缓冲区
    @param [in] type 四字节PNG块类型
    @param [in] data 输入数据
    @return 是否满足条件或操作成功。
    */
    static bool appendChunk(std::vector<uint8_t>& bytes,
    const uint8_t* type,
    const std::vector<uint8_t>& data);
    /** 在PNG结束块前插入配置块。
    @param [in] thumbnailPng PNG预览字节
    @param [in] configBytes UTF-16LE配置字节
    @param [in,out] packageBytes 封装结果
    @return 是否满足条件或操作成功。
    */
    static bool buildConfigPackage(const std::vector<uint8_t>& thumbnailPng,
    const std::vector<uint8_t>& configBytes,
    std::vector<uint8_t>& packageBytes);
    /** 从PNG配置块或旧版尾部标记提取配置。
    @param [in] bytes 字节缓冲区
    @param [in,out] configBytes UTF-16LE配置字节
    @return 是否满足条件或操作成功。
    */
    static bool extractEmbeddedConfig(const std::vector<uint8_t>& bytes,
    std::vector<uint8_t>& configBytes);
};