#pragma once

#include <stdint.h>
#include <string>
#include <vector>

/** PdfEngine工程内部通用辅助函数集合
*/
class PdfEngineHelper
{
public:
    /** 写入接口错误文本
    @param [out] errorText 可选错误输出对象
    @param [in] value 要写入的错误文本
    */
    static void setErrorText(std::string* errorText, const std::string& value);

    /** 裁剪ASCII文本首尾空白字符
    @param [in] value 待裁剪的ASCII文本
    @return 去掉首尾空白字符后的文本
    */
    static std::string trimAscii(const std::string& value);

    /** 解析严格正整数页码
    @param [in] value 用户输入的页码文本
    @param [out] number 不为空时写入解析后的正整数页码
    @return 文本是合法正整数页码时返回true，否则返回false
    */
    static bool parsePositivePageNumber(const std::string& value, int32_t* number);

    /** 解析用户输入的页码范围文本
    @param [in] rangeText 页码范围文本，例如3-5,7-8
    @param [in] pageCount 当前PDF总页数
    @param [out] pageIndexes 解析后的0基页面索引列表
    @param [out] errorText 解析失败原因，可为空
    @return 解析成功返回true，否则返回false
    */
    static bool parsePageRanges(const std::string& rangeText,
                                int32_t pageCount,
                                std::vector<int32_t>* pageIndexes,
                                std::string* errorText = nullptr);

    /** 将刚失败的PDFium调用错误转换成本库错误文本
    @return 返回可读错误文本，应在PDFium接口失败后立刻调用
    */
    static std::string pdfiumErrorText();
};