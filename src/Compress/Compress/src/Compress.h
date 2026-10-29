#pragma once
#include "CompressMacro.h"
#include <stdint.h>
#include <string>

// 原始zlib流操作的统一结果，不向调用方暴露第三方错误码
enum CompressResult
{
    // 操作成功，长度参数为实际结果
    COMPRESS_OK = 0,
    // 指针、输入长度或压缩等级无效
    COMPRESS_INVALID_ARGUMENT = -1,
    // 输出缓冲区不足
    COMPRESS_BUFFER_TOO_SMALL = -2,
    // 压缩器分配内存失败
    COMPRESS_NO_MEMORY = -3,
    // 输入流损坏或未完整结束
    COMPRESS_DATA_ERROR = -4
};

class CompressAPI Compress
{
public:
    /** 估算不附加长度头的zlib流压缩容量
    @param [in] srcLength 原始数据字节数
    @return 所需容量上界，溢出或饱和、无法保证容量时返回0
    */
    static uint32_t zlibRawCompressBound(uint32_t srcLength);

    /** 压缩原始字节，不添加Compress历史接口的四字节长度头
    @param [out] dest 输出缓冲区，不能为空，不得与输入重叠
    @param [in,out] destLength 输入为容量，成功时为输出字节数，失败时为0
    @param [in] src 借用的输入数据，不能为空
    @param [in] srcLength 输入字节数
    @param [in] level 压缩等级，范围0到9
    @return CompressResult结果码，同步完成
    */
    static int32_t zlibRawCompress(uint8_t* dest, uint32_t& destLength, const uint8_t* src, uint32_t srcLength, int32_t level = 7);

    /** 解压原始zlib流并返回实际消费的输入长度，供协议拒绝尾随数据
    @param [out] dest 输出缓冲区，不能为空，不得与输入重叠
    @param [in,out] destLength 输入为容量，成功时为解压字节数，失败时为0
    @param [in] src 借用的压缩数据，不能为空
    @param [in,out] srcLength 输入为压缩字节数，成功时为实际消费字节数，失败时为0
    @return CompressResult结果码，同步完成，失败时输出缓冲区内容不可用
    */
    static int32_t zlibRawUnCompress(uint8_t* dest, uint32_t& destLength, const uint8_t* src, uint32_t& srcLength);

    /** 压缩并添加四字节原长头，保持历史字节序
    @param [out] dest 输出缓冲区，不得与输入重叠
    @param [in,out] destLength 输入为包括长度头的总容量，成功为包括长度头的总字节数，失败为0
    @param [in] src 非空的输入数据
    @param [in] srcLength 输入字节数，必须大于0
    @param [in] level 压缩等级，范围0到9
    */
    static void zlibCompress(char* dest, uint32_t& destLength, const char* src, uint32_t srcLength, int32_t level = 7);

    /** 估算包含四字节原长头的压缩容量
    @param [in] srcLength 原始数据字节数
    @return 压缩总容量上界，溢出或饱和时为0
    */
    static uint32_t zlibCompressBound(uint32_t srcLength);

    /** 读取历史格式的四字节原长，结果不代表压缩数据有效
    @param [in] src 至少可读四字节的长度头指针，nullptr时返回0
    @return 头中声明的原长，解压时仍须校验真实结果
    */
    static uint32_t zlibUnCompressBound(const char* src);

    /** 解压带四字节原长头的数据，验证原长和输入完整性
    @param [out] dest 输出缓冲区，不得与输入重叠，失败时内容不可用
    @param [in,out] destLength 输入为输出容量，成功为实际原长，失败为0
    @param [in] src 包括长度头的压缩数据
    @param [in] srcLength 包括长度头的总字节数，必须大于4
    */
    static void zlibUnCompress(char* dest, uint32_t& destLength, const char* src, uint32_t srcLength);

    /** 压缩字符串为历史带长度头格式
    @param [in] src 输入字节串，长度不超过uint32_t范围
    @param [in] level 压缩等级，范围0到9
    @return 压缩字节串，空输入、非法参数或压缩失败时为空
    */
    static std::string zlibCompress(const std::string& src, int32_t level = 7);

    /** 解压历史带长度头格式并验证原长、校验和及尾随数据
    @param [in] src 包括长度头的压缩字节串
    @return 解压字节串，输入无效或解压失败时为空
    */
    static std::string zlibUnCompress(const std::string& src);

    /** 压缩字符串为历史带长度头格式，支持输入输出为同一对象
    @param [out] dest 压缩结果，空输入或失败时清空
    @param [in] src 输入字节串，长度不超过uint32_t范围
    @param [in] level 压缩等级，范围0到9
    */
    static void zlibCompress(std::string& dest, const std::string& src, int32_t level = 7);

    /** 解压历史带长度头格式，支持输入输出为同一对象
    @param [out] dest 解压结果，无效输入或失败时清空
    @param [in] src 包括长度头的压缩字节串
    */
    static void zlibUnCompress(std::string& dest, const std::string& src);
};