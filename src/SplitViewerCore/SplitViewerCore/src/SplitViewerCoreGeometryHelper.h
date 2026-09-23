#pragma once
#include "SplitViewerCoreNode.h"
#include "SplitViewerCoreRect.h"

/** Geometry and image transforms. */
class SplitViewerCoreGeometryHelper
{
public:
    /** 将数值限制在闭区间内。
    @param [in] value 输入值
    @param [in] low 下限
    @param [in] high 上限
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static double clampDouble(double value, double low, double high);
    /** 计算两个子区域与分割线区域。
    @param [in] owner 父区域或事件接收者
    @param [in] node 文档持有的节点
    @param [in,out] first 第一子区域
    @param [in,out] splitter 分割线区域
    @param [in,out] second 第二子区域
    @param [in] thickness 分割线厚度
    */
    static void splitNodeRects(const SplitViewerCoreRect& owner,
    const SplitViewerCoreNode* node,
    SplitViewerCoreRect& first,
    SplitViewerCoreRect& splitter,
    SplitViewerCoreRect& second,
    double thickness);
    /** 限制图层尺寸和位置，使其位于工作区内。
    @param [in,out] rect 区域矩形
    @param [in] bounds 边界矩形
    @param [in] minWidth 最小宽度
    @param [in] minHeight 最小高度
    */
    static void constrainLayerRect(SplitViewerCoreRect& rect,
    const SplitViewerCoreRect& bounds,
    double minWidth,
    double minHeight);
    /** 计算保持宽高比的图片适配比例。
    @param [in] imageWidth 图片宽度
    @param [in] imageHeight 图片高度
    @param [in] contentWidth 内容宽度
    @param [in] contentHeight 内容高度
    @return 计算结果或借用对象，具体语义见函数说明。
    */
    static double fitScale(double imageWidth, double imageHeight, double contentWidth, double contentHeight);
    /** 按照滚轮步长更新图片缩放。
    @param [in,out] view 图片视图状态
    @param [in] fitScale 自动适配比例
    @param [in] delta 滚轮角度
    @param [in] fine 是否精细缩放
    */
    static void zoom(SplitViewerCoreLeafState& view, double fitScale, int delta, bool fine);
    /** 按内容尺寸变化调整图片偏移与手动比例。
    @param [in,out] view 图片视图状态
    @param [in] oldWidth 原内容宽度
    @param [in] oldHeight 原内容高度
    @param [in] newWidth 新内容宽度
    @param [in] newHeight 新内容高度
    */
    static void resizeView(SplitViewerCoreLeafState& view, double oldWidth, double oldHeight, double newWidth, double newHeight);
};