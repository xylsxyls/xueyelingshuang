#pragma once
#include "CompressMacro.h"
#include <stdint.h>
#include <string>

// Zstandard帧操作的统一结果，不向调用方暴露第三方错误码
enum CompressResult
{
    // 操作成功，长度参数为实际结果
    COMPRESS_OK = 0,
    // 指针、长度参数或压缩等级无效
    COMPRESS_INVALID_ARGUMENT = -1,
    // 输出缓冲区不足
    COMPRESS_BUFFER_TOO_SMALL = -2,
    // 编解码器分配内存失败
    COMPRESS_NO_MEMORY = -3,
    // 输入流损坏或未完整结束
    COMPRESS_DATA_ERROR = -4
};

// 同步、无实例状态的 Zstandard 编解码接口；压缩生成带校验和的数据帧
// 缓冲区只在调用期间借用。字符串重载申请标准库存储时可能抛出异常
class CompressAPI Compress
{
public:
    /** 估算不附加长度头的Zstandard帧压缩容量
    @param [in] srcLength 原始数据字节数
    @return 所需容量上界，溢出或饱和、无法保证容量时返回0
    */
    static uint32_t rawCompressBound(uint32_t srcLength);

    /** 将原始字节压缩为单个标准数据帧
    @param [out] dest 输出缓冲区，不能为空，不得与输入重叠
    @param [in,out] destLength 输入为容量，成功时为输出字节数，失败时为0
    @param [in] src 借用的输入数据，不能为空
    @param [in] srcLength 输入字节数，可为0
    @param [in] level 压缩等级，范围0到9，0采用Zstandard默认等级
    @return CompressResult结果码，同步完成
    */
    static int32_t rawCompress(uint8_t* dest, uint32_t& destLength, const uint8_t* src, uint32_t srcLength, int32_t level = 7);

    /** 解压Zstandard帧并返回实际消费的输入长度，供协议拒绝尾随数据
    @param [out] dest 输出缓冲区，不能为空，不得与输入重叠
    @param [in,out] destLength 输入为容量，成功时为解压字节数，失败时为0
    @param [in] src 借用的压缩数据，不能为空
    @param [in,out] srcLength 输入为可用压缩字节数，成功时为首帧的消费量，失败时为0；须与destLength使用不同变量
    @return CompressResult结果码，同步完成，失败时输出缓冲区内容不可用
    */
    static int32_t rawUncompress(uint8_t* dest, uint32_t& destLength, const uint8_t* src, uint32_t& srcLength);

    /** 压缩并添加本机字节序的四字节原长头
    @param [out] dest 非空输出缓冲区，不得与输入重叠
    @param [in,out] destLength 输入为包括长度头的总容量，成功为包括长度头的总字节数，失败为0
    @param [in] src 非空的输入数据
    @param [in] srcLength 输入字节数，必须大于0
    @param [in] level 压缩等级，范围0到9，0采用Zstandard默认等级
    */
    static void compress(char* dest, uint32_t& destLength, const char* src, uint32_t srcLength, int32_t level = 7);

    /** 估算包含四字节原长头的压缩容量
    @param [in] srcLength 原始数据字节数
    @return 压缩总容量上界，溢出或饱和时为0
    */
    static uint32_t compressBound(uint32_t srcLength);

    /** 读取封装帧的四字节原长，调用方还须验证帧内容
    @param [in] src 至少可读四字节的长度头指针，nullptr时返回0
    @return 头中声明的原长，解压时仍须校验真实结果
    */
    static uint32_t uncompressBound(const char* src);

    /** 解压带四字节原长头的数据，验证原长和输入完整性
    @param [out] dest 非空输出缓冲区，不得与输入重叠，失败时内容不可用
    @param [in,out] destLength 输入为输出容量，成功为实际原长，失败为0
    @param [in] src 包括长度头的非空压缩数据
    @param [in] srcLength 包括长度头的总字节数，必须大于4
    */
    static void uncompress(char* dest, uint32_t& destLength, const char* src, uint32_t srcLength);

    /** 压缩字符串为带长度头的Zstandard格式
    @param [in] src 输入字节串，长度不超过uint32_t范围
    @param [in] level 压缩等级，范围0到9，0采用Zstandard默认等级
    @return 压缩字节串，空输入、非法参数或压缩失败时为空
    */
    static std::string compress(const std::string& src, int32_t level = 7);

    /** 解压带长度头的标准帧，验证原长、帧中携带的校验和及输入完整性
    @param [in] src 包括长度头的压缩字节串
    @return 解压字节串，输入无效或解压失败时为空
    */
    static std::string uncompress(const std::string& src);

    /** 压缩字符串为带长度头的Zstandard格式，支持输入输出为同一对象
    @param [out] dest 压缩结果，空输入或失败时清空
    @param [in] src 输入字节串，长度不超过uint32_t范围
    @param [in] level 压缩等级，范围0到9，0采用Zstandard默认等级
    */
    static void compress(std::string& dest, const std::string& src, int32_t level = 7);

    /** 解压带长度头的Zstandard格式，支持输入输出为同一对象
    @param [out] dest 解压结果，无效输入或失败时清空
    @param [in] src 包括长度头的压缩字节串
    */
    static void uncompress(std::string& dest, const std::string& src);

    /** 解压带原长头的字符串，在任何输出分配前检查调用方指定的上限
    @param [out] dest 解压结果，允许与 src 相同。输入或上限无效、结果越限时清空
    @param [in] src 带四字节原长头的完整 Zstandard 帧
    @param [in] maxOutputBytes 最大明文字节数，-1 不设业务上限，0 仅允许空输出，小于-1时清空目标
    */
    static void uncompress(std::string& dest, const std::string& src, int64_t maxOutputBytes);
};