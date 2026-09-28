#pragma once
#include "PdfReaderCoreMacro.h"
#include <stdint.h>

/** 核心初始化配置；init 校验并复制，各实例独立，运行中只读
*/
class PdfReaderCoreAPI PdfReaderCoreConfig
{
public:
    // Core渲染单页位图时允许的宽×高上限，单位为像素个数，范围1..268435456；超限返回错误而不分配位图
    uint64_t m_maxRenderPixels;
    // 逐页导出文件名中页码的最少十进制位数，范围1..9；不足时左补0，例如3生成_001.pdf，不截断更长页码
    int32_t m_exportNumberWidth;
public:
    /** 默认 6400 万像素、三位序号，不创建引擎或读取全局配置
    */
    PdfReaderCoreConfig();

    /** 返回全部字段是否处于支持范围
    */
    bool isValid() const;
};