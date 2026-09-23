#pragma once
#include "PdfReaderCore.h"
#include "PdfReaderCoreC.h"

/** 每个 C 句柄拥有一份 Core 和调用结果；没有全局会话状态 */
struct PdfReaderCoreCContext
{
public:
    PdfReaderCore core;
    int32_t lastResult;
    std::string lastError;
public:
    /** 建立未初始化上下文，实际引擎由 Init 获取 */
    PdfReaderCoreCContext();
};