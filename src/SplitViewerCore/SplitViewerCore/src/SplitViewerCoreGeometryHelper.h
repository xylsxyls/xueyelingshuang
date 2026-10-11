#pragma once
#include "SplitViewerCoreNode.h"
#include "SplitViewerCoreRect.h"
#include <stdint.h>

/** 无状态几何和图片变换算法；只修改调用方明确传入的矩形或视图值
*/
class SplitViewerCoreGeometryHelper
{
public:
    /** 将数值限制在闭区间内
    @param [in] value 输入值
    @param [in] low 下限
    @param [in] high 上限
    @return value小于下限时取low，大于上限时取high，否则保持value
    */
    static double clampDouble(double value, double low, double high);

    /** 计算两个子区域与分割线区域
    @param [in] owner 父内容矩形，与输出区域使用同一坐标单位
    @param [in] node 借用的分割节点；空指针使用默认比例并按水平分割计算
    @param [out] first 第一子区域，上方或左方，不含分割线
    @param [out] splitter 分割线矩形，与父区域使用同一坐标系
    @param [out] second 第二子区域，下方或右方，不含分割线
    @param [in] thickness 分割线厚度，与父区域同单位，限制到对应边长的三分之一
    */
    static void splitNodeRects(const SplitViewerCoreRect& owner, const SplitViewerCoreNode* node,
        SplitViewerCoreRect& first, SplitViewerCoreRect& splitter, SplitViewerCoreRect& second, double thickness);

    /** 限制图层尺寸和位置，使其位于工作区内
    @param [in,out] rect 区域矩形
    @param [in] bounds 边界矩形
    @param [in] minWidth 最小宽度
    @param [in] minHeight 最小高度
    */
    static void constrainLayerRect(
        SplitViewerCoreRect& rect, const SplitViewerCoreRect& bounds, double minWidth, double minHeight);

    /** 计算保持宽高比的图片适配比例
    @param [in] imageWidth 图片宽度
    @param [in] imageHeight 图片高度
    @param [in] contentWidth 内容宽度
    @param [in] contentHeight 内容高度
    @return 两个方向适配比例的较小值；尺寸非有限或非正时返回1.0
    */
    static double fitScale(double imageWidth, double imageHeight, double contentWidth, double contentHeight);

    /** 按照滚轮步长更新图片缩放
    @param [in,out] view 图片视图状态
    @param [in] fitScale 自动适配比例
    @param [in] delta Qt滚轮angleDelta单位，120表示一个标准刻度，0不改变状态
    @param [in] fine 是否精细缩放
    @param [in] config 本实例复制的有效缩放策略
    */
    static void zoom(SplitViewerCoreLeafState& view, double fitScale, int32_t delta, bool fine,
        const SplitViewerCoreConfig& config = SplitViewerCoreConfig());

    /** 按内容尺寸变化调整图片偏移与手动比例
    @param [in,out] view 图片视图状态
    @param [in] oldWidth 原内容宽度
    @param [in] oldHeight 原内容高度
    @param [in] newWidth 新内容宽度
    @param [in] newHeight 新内容高度
    */
    static void resizeView(
        SplitViewerCoreLeafState& view, double oldWidth, double oldHeight, double newWidth, double newHeight);
};