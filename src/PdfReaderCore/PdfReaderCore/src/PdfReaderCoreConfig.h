#pragma once
#include "PdfReaderCoreMacro.h"
#include <stdint.h>

/** 核心初始化配置；init 校验并复制，各实例独立，运行中只读 */
class PdfReaderCoreAPI PdfReaderCoreConfig
{
public:
    // 单次渲染最大像素数，范围 1..268435456，限制分配和乘法溢出
    uint64_t maxRenderPixels;
    // 分页导出序号最少位数，范围 1..9，不截断更长页码
    int32_t exportNumberWidth;
public:
    /** 默认 6400 万像素、三位序号，不创建引擎或读取全局配置 */
    PdfReaderCoreConfig();
    /** 返回全部字段是否处于支持范围 */
    bool isValid() const;
};