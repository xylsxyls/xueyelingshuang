#pragma once

#include "PdfEngineMacro.h"

#include <stdint.h>

class PdfDocument;

/** 保存PDF时使用的页面引用
*/
struct PdfEngineAPI PdfEnginePageRef
{
public:
    // 页面所在的PDF文档对象，不负责释放
    PdfDocument* document;
    // 页面在原始PDF文档中的0基索引
    int32_t pageIndex;

public:
    /** 构造无效页面引用
    */
    PdfEnginePageRef();

    /** 构造指向指定PDF文档页面的引用
    @param [in] doc 页面所在的PDF文档对象
    @param [in] indexInDocument 页面在原始PDF文档中的0基索引
    */
    PdfEnginePageRef(PdfDocument* doc, int32_t indexInDocument);
};