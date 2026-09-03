#pragma once

#include "PdfEngineMacro.h"

#include <stdint.h>
#include <vector>

/** PDF页面渲染后的BGRA位图
*/
struct PdfEngineAPI PdfEngineBitmap
{
public:
    // 位图宽度，单位为像素
    int32_t width;
    // 位图高度，单位为像素
    int32_t height;
    // 位图每行字节数
    int32_t stride;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // BGRA像素缓冲区
    std::vector<unsigned char> pixels;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

public:
    /** 构造空位图
    */
    PdfEngineBitmap();

    /** 清空当前位图内容
    */
    void clear();
};