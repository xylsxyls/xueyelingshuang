#pragma once
#include <string>
#include <stdint.h>
#include <stddef.h>

/** 白盒注入规则；上下文必须在当前生产源码中恰好出现一次
*/
struct SemaphoreTestInjection
{
    // 被测源文件名
    const char* file;
    // 按UTF-8字节匹配的相邻生产代码，不含测试标记
    const char* context;
    // 插入点相对上下文起点的字节数
    size_t offset;
    // 仅加入临时白盒副本的测试代码
    const char* text;
};

/** 由测试工程构建的宿主工具，从真实源码生成白盒副本，不改写生产目录
*/
class SemaphoreTestInstrument
{
public:
    /** 生成四个临时白盒源文件，全部规则验证成功后才开始输出
    @param [in] sourceDirectory 生产源码目录
    @param [in] outputDirectory 已存在且不同于生产目录的临时输出目录
    @return 0完成，非0为输入、规则或写入失败；异常由main报告
    */
    static int32_t run(const std::string& sourceDirectory, const std::string& outputDirectory);

    /** 验证正常注入以及每个上下文缺失、重复时的拒绝行为
    @param [in] sourceDirectory 当前生产源码目录
    @return 0检查通过，非0为失败
    */
    static int32_t check(const std::string& sourceDirectory);

private:
    /** 取得测试工程维护的注入规则
    @param [out] count 规则数量
    @return 静态规则数组，调用方不释放
    */
    static const SemaphoreTestInjection* rules(size_t& count);

    /** 读取文件，去除UTF-8 BOM并将CRLF统一为LF以匹配上下文
    @param [in] path 文件路径
    @param [out] text 规范化后的UTF-8字节
    @return 是否读取成功
    */
    static bool read(const std::string& path, std::string& text);

    /** 写入带BOM、CRLF的临时C++文件；内容未变时保留时间戳
    @param [in] path 目标文件路径
    @param [in] text LF格式的UTF-8内容
    @return 是否写入并关闭成功
    */
    static bool write(const std::string& path, const std::string& text);

    /** 校验唯一上下文后按位置插入，生产算法的原始字节全部保留
    @param [in] file 被测文件名
    @param [in] source LF格式生产源码
    @param [out] result 注入后的源码；失败时不修改
    @param [in] report 是否输出失败规则
    @return 所有对应规则均成功匹配且插入点无冲突
    */
    static bool instrument(const char* file, const std::string& source, std::string& result, bool report);

    /** 解析已存在目录的真实路径，拒绝输出覆盖生产目录
    @param [in] path 目录路径
    @param [out] resolved 已解析的路径
    @return 是否解析并释放系统资源成功
    */
    static bool resolveDirectory(const std::string& path, std::string& resolved);
};