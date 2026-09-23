#pragma once
#include <stdint.h>
#include <string>
class PdfDocument;

/** 页面来源。document 由 Core 文档集合持有，在会话关闭前有效 */
struct PdfReaderCorePageEntry
{
    PdfDocument* document;
    int32_t pageIndex;
    std::wstring sourcePath;
};