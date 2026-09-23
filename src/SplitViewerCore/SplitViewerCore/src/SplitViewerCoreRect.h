#pragma once
#include "SplitViewerCoreMacro.h"
#include <string>
#include <vector>

struct SplitViewerCoreAPI SplitViewerCoreRect
{
    double left;
    double top;
    double right;
    double bottom;

    /** 初始化对象及其默认状态。
    */
    SplitViewerCoreRect();
    /** 初始化对象及其默认状态。
    */
    SplitViewerCoreRect(double leftValue, double topValue, double rightValue, double bottomValue);
    /** 返回矩形宽度。
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    double width() const;
    /** 返回矩形高度。
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    double height() const;
};