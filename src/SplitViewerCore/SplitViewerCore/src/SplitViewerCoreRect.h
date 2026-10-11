#pragma once
#include "SplitViewerCoreMacro.h"

/** 无资源的矩形值模型，边界使用调用方约定的同一坐标系
*/
struct SplitViewerCoreAPI SplitViewerCoreRect
{
public:
    // 左边界，单位由调用方坐标系决定
    double left;
    // 上边界，单位由调用方坐标系决定
    double top;
    // 右边界
    double right;
    // 下边界
    double bottom;

public:
    /** 初始化对象及其默认状态
    */
    SplitViewerCoreRect();

    /** 设置指定矩形边界，不额外校正边界顺序
    @param [in] leftValue 左边界
    @param [in] topValue 上边界
    @param [in] rightValue 右边界
    @param [in] bottomValue 下边界，四项须使用相同单位
    */
    SplitViewerCoreRect(double leftValue, double topValue, double rightValue, double bottomValue);

    /** 返回矩形宽度
    @return 返回矩形宽度
    */
    double width() const;

    /** 返回矩形高度
    @return 返回矩形高度
    */
    double height() const;
};