#pragma once

#include "PdfReaderConstants.h"

/** 自绘标题栏或工具栏按钮状态
*/
struct TopButtonState
{
public:
    // 按钮ID，对应TopButton枚举
    int id;
    // 按钮在主窗口客户区中的矩形区域
    RECT rect;
    // 按钮当前是否可用
    bool enabled;

public:
    /** 构造不可用的空按钮状态
    */
    TopButtonState();
};