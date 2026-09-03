#pragma once

#include "PdfEngineMacro.h"

/** PDF页面尺寸，单位为PDF point
*/
struct PdfEngineAPI PdfEnginePageSize
{
public:
    // 页面宽度
    double width;
    // 页面高度
    double height;

public:
    /** 构造空页面尺寸
    */
    PdfEnginePageSize();
};