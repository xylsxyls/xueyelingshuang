#pragma once
#include "PdfEngine/PdfEngineAPI.h"
#include <string>
#include <vector>

/** 无状态导出辅助：统一命名与临时写入/替换协议；引擎只在调用期间借用 */
class PdfReaderCoreExportHelper
{
public:
    /** 生成带小写 .pdf 后缀的分页路径；directory 已含末尾分隔符 */
    static std::string pageFileName(const std::string& directory, const std::string& prefix, int32_t page, int32_t numberWidth);
    /** 同目录临时写入后替换，替换失败保留恢复文件并输出错误 */
    static bool savePages(PdfEngine& engine, const std::wstring& outputPath,
        const std::vector<PdfEnginePageRef>& refs, std::string* errorText);
};