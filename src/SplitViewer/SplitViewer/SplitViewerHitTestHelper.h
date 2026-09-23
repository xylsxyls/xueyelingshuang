#pragma once
#include "SplitViewerHit.h"
/** 根据明确的布局输入查询命中结果，不持有窗口或改变文档。 */
class SplitViewerHitTestHelper
{
public:
    /** 按图层层叠顺序查询当前命中区域。
    @param [in] document 文档模型
    @param [in] stage 背景板像素矩形
    @param [in] point 工作区坐标
    @param [in,out] hit 命中结果
    @return 是否满足条件或操作成功。
    */
    static bool hitAll(const SplitViewerCoreDocument& document, const QRectF& stage, const QPointF& point, SplitViewerHit& hit);
private:
    /** 在分屏树内查询叶节点或分割线。
    @param [in] node 文档持有的节点
    @param [in] rect 区域矩形
    @param [in] point 工作区坐标
    @param [in] layerIndex 图层索引
    @param [in,out] hit 命中结果
    @param [in] borderVisible 是否显示边框
    @return 是否满足条件或操作成功。
    */
    static bool hitNode(SplitViewerCoreNode* node, const QRectF& rect, const QPointF& point, int layerIndex, SplitViewerHit& hit, bool borderVisible);
};