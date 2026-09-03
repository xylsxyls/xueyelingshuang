#pragma once

#include "PdfReaderPlatform.h"

/** 页面渲染缓存项
*/
struct BitmapCacheEntry
{
public:
    // 缓存所属PDF文档对象，不负责释放
    PdfDocument* document;
    // 页面在原始PDF文档中的0基索引
    int pageIndex;
    // 缓存位图宽度，单位为像素
    int width;
    // 缓存位图高度，单位为像素
    int height;
    // 页面渲染后的BGRA位图
    PdfEngineBitmap bitmap;

public:
    /** 构造空缓存项
    */
    BitmapCacheEntry();
};