#pragma once
#include <stdint.h>
#include <string>

struct PdfReaderCoreCContext;

/** C 边界结果转换；不持有上下文，不管理引擎生命周期
*/
class PdfReaderCoreCResultHelper
{
public:
    /** 按引擎错误文字细分 C 结果码
    @param [in] fallback 没有匹配错误类别时的返回码
    @param [in] error UTF-8 错误文字
    @return 细分后的 C 结果码
    */
    static int32_t classifyError(int32_t fallback, const std::string& error);

    /** 记录调用结果，成功时清除旧错误
    @param [in,out] context 借用上下文，允许为空且不保存
    @param [in] result 本次结果码
    @return result 原值
    */
    static int32_t runResult(PdfReaderCoreCContext* context, int32_t result);

    /** 保存错误并返回细分后的结果码
    @param [in,out] context 借用上下文，为空时返回 InvalidParam
    @param [in] result 默认错误码
    @param [in] error 复制保存的 UTF-8 错误文字
    @return 细分后的错误码，或 InvalidParam
    */
    static int32_t runError(PdfReaderCoreCContext* context, int32_t result, const std::string& error);
};