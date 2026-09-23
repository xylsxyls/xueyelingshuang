#include "SplitViewerHitTestHelper.h"
#include "SplitViewerLayoutHelper.h"
#include <algorithm>

bool SplitViewerHitTestHelper::hitNode(SplitViewerCoreNode* node, const QRectF& rect, const QPointF& point, int layerIndex, SplitViewerHit& hit, bool borderVisible)
{
    if (!node || !rect.contains(point))
    {
        return false;
    }
    if (node->isLeaf())
    {
        hit.node = node;
        hit.layer = layerIndex;
        hit.rect = rect;
        hit.ownerRect = rect;
        hit.splitter = false;
        return true;
    }
    QRectF first, splitterRect, second;
    SplitViewerLayoutHelper::nodeRects(rect, node, first, splitterRect, second, borderVisible);
    if (splitterRect.adjusted(-4, -4, 4, 4).contains(point))
    {
        hit.node = node;
        hit.layer = layerIndex;
        hit.rect = splitterRect;
        hit.ownerRect = rect;
        hit.splitter = true;
        return true;
    }
    if (hitNode(node->first, first, point, layerIndex, hit, borderVisible))
    {
        return true;
    }
    return hitNode(node->second, second, point, layerIndex, hit, borderVisible);
}

bool SplitViewerHitTestHelper::hitAll(const SplitViewerCoreDocument& document, const QRectF& stage, const QPointF& point, SplitViewerHit& hit)
{
    for (int i = document.layerCount() - 1; i >= 0; --i)
    {
        SplitViewerCoreLayer* layer = document.layerAt(i);
        const QRectF owner=SplitViewerLayoutHelper::normalizedToPixel(layer->rect,stage);
        const QRectF inner=SplitViewerLayoutHelper::contentRect(owner, document.borderVisible());
        const QPointF innerPoint((std::max)(inner.left(),(std::min)(inner.right(),point.x())),
            (std::max)(inner.top(),(std::min)(inner.bottom(),point.y())));
        if (owner.contains(point) && hitNode(layer->root,inner,innerPoint,i,hit,document.borderVisible()))
        {
            hit.root = layer->root;
            hit.layerRect = SplitViewerLayoutHelper::normalizedToPixel(layer->rect, stage);
            return true;
        }
    }
    hit.root = document.baseRoot();
    return hitNode(document.baseRoot(), SplitViewerLayoutHelper::contentRect(stage, document.borderVisible()), point, -1, hit, document.borderVisible());
}