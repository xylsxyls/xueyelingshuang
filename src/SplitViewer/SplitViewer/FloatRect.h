#pragma once

#include "SplitViewerCommon.h"

/** 归一化矩形，保存浮动图层相对底部舞台区域的位置和大小
*/
struct SplitViewerFloatRect
{
public:
    // 左侧相对坐标，0表示舞台左边缘
    double left;
    // 顶部相对坐标，0表示舞台上边缘
    double top;
    // 右侧相对坐标，1表示舞台右边缘
    double right;
    // 底部相对坐标，1表示舞台下边缘
    double bottom;

public:
    /** 构造默认空矩形
    */
    SplitViewerFloatRect();
};