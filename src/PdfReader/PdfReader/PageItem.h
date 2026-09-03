#pragma once

#include "PdfReaderPlatform.h"

/** PDFReader页面顺序列表中的一个页面
*/
struct PageItem
{
public:
    // 页面所在的PDF文档对象，不负责释放
    PdfDocument* document;
    // 页面在原始PDF文档中的0基索引
    int pageIndex;
    // 页面来源PDF路径
    std::wstring sourcePath;

public:
    /** 构造空页面项
    */
    PageItem();
};