#pragma once
#include "PdfEngine/PdfEngineAPI.h"
#include <string>
#include <vector>

/** 无状态导出辅助：统一命名与临时写入/替换协议；引擎只在调用期间借用
*/
class PdfReaderCoreExportHelper
{
public:
    /** 生成带小写.pdf后缀的分页路径，目录分隔符由CSystem统一拼接
    @param [in] directory 输出目录，可带或不带末尾分隔符
    @param [in] prefix 经过Core校验的文件名前缀，不含路径分隔符
    @param [in] page 从1开始的输出页号
    @param [in] numberWidth 页号最小十进制位数
    @return UTF8完整输出路径
    */
    static std::string pageFileName(const std::string& directory, const std::string& prefix, int32_t page, int32_t numberWidth);

    /** 同目录临时写入后替换，替换失败保留恢复文件并输出错误
    */
    static bool savePages(PdfEngine& engine, const std::wstring& outputPath,
        const std::vector<PdfEnginePageRef>& refs, std::string* errorText);
};